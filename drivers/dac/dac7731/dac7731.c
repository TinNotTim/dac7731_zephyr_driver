/*
 * Copyright (c) 2024, Tin Chiang
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>


/* Use the Instance-based APIs*/
#define DT_DRV_COMPAT ti_dac7731

/* Register the module to logging submodule*/
// LOG_MODULE_REGISTER(DAC7731, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(DAC7731, CONFIG_SPI_LOG_LEVEL);

/* Define the device data and config struct*/
struct dac7731_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset_gpios;
	struct gpio_dt_spec ldac_gpios;
};

struct dac7731_data {
};


static int dac7731_config_reset(const struct device *dac7731)
{
	const struct dac7731_config *config = dac7731->config;
	int ret;

	/* Check if the gpio port exists */
    if(config->reset_gpios.port == NULL){
        LOG_ERR("gpio_drdy not defined in DT");
        return -ENODEV;
    }

    /* Check if the gpio port is ready */
    if (!gpio_is_ready_dt(&config->reset_gpios)) {
		LOG_ERR("device %s is not ready", config->reset_gpios.port->name);
		return -ENODEV;
	}

    /* set up reset gpio as output */
    ret = gpio_pin_configure_dt(&config->reset_gpios, GPIO_OUTPUT|GPIO_ACTIVE_LOW);
    if(ret != 0){
        LOG_ERR("ret:%d, Setting %s to output fail", ret, config->reset_gpios.port->name);
        return ret;
    }

	/* reset the pin state to 0(high)*/
	ret = gpio_pin_set_dt(&config->reset_gpios, 0);
	if(ret != 0){
        LOG_ERR("ret:%d, Setting %s to output fail", ret, config->reset_gpios.port->name);
        return ret;
    }

	/* successful*/
	LOG_DBG("Successfully configure the reset pin on gpio.");
	return 0;
}

static int dac7731_config_ldac(const struct device *dac7731)
{
	const struct dac7731_config *config = dac7731->config;
	int ret;

	/* Check if the gpio port exists */
    if(config->ldac_gpios.port == NULL){
        LOG_ERR("gpio_drdy not defined in DT");
        return -ENODEV;
    }

    /* Check if the gpio port is ready */
    if (!gpio_is_ready_dt(&config->ldac_gpios)) {
		LOG_ERR("device %s is not ready", config->ldac_gpios.port->name);
		return -ENODEV;
	}

    /* set up LDAC gpio as output */
    ret = gpio_pin_configure_dt(&config->ldac_gpios, GPIO_OUTPUT|GPIO_ACTIVE_HIGH);
    if(ret != 0){
        LOG_ERR("ret:%d, Setting %s to output fail", ret, config->ldac_gpios.port->name);
        return ret;
    }

	/* reset the pin state to 0(low)*/
	ret = gpio_pin_set_dt(&config->ldac_gpios, 0);
	if(ret != 0){
        LOG_ERR("ret:%d, Setting %s to output fail", ret, config->ldac_gpios.port->name);
        return ret;
    }

	/* successful*/
	LOG_DBG("Successfully configure the reset pin on gpio.");
	return 0;

}

static int dac7731_reset(const struct device *dac7731)
{
	const struct dac7731_config *config = dac7731->config;
	int ret;

	/* Check if the gpio port is ready */
    if (!gpio_is_ready_dt(&config->reset_gpios)) {
		LOG_ERR("device %s is not ready", config->reset_gpios.port->name);
		return -ENODEV;
	}

	/* set the pin state to 1(low)*/
	ret = gpio_pin_set_dt(&config->reset_gpios, 1);
	if(ret != 0){
        LOG_ERR("ret:%d, Set reset pin fail", ret);
        return ret;
    }

	/* reset the pin state to 0(high)*/
	ret = gpio_pin_set_dt(&config->reset_gpios, 0);
	if(ret != 0){
        LOG_ERR("ret:%d, Reset reset pin fail", ret);
        return ret;
    }

	/* successful*/
	LOG_DBG("Successfully reset the chip.");
	return 0;
}

static int dac7731_load_value(const struct device *dac7731)
{
	const struct dac7731_config *config = dac7731->config;
	int ret;

	/* Check if the gpio port is ready */
    if (!gpio_is_ready_dt(&config->ldac_gpios)) {
		LOG_ERR("device %s is not ready", config->ldac_gpios.port->name);
		return -ENODEV;
	}

	/* set the pin state to 1(high)*/
	ret = gpio_pin_set_dt(&config->ldac_gpios, 1);
	if(ret != 0){
        LOG_ERR("ret:%d, Set ldac pin fail", ret);
        return ret;
    }

	/* reset the pin state to 0(low)*/
	ret = gpio_pin_set_dt(&config->ldac_gpios, 0);
	if(ret != 0){
        LOG_ERR("ret:%d, Reset ldac pin fail", ret);
        return ret;
    }

	/* successful*/
	LOG_DBG("Successfully load the value.");
	return 0;
}

/* Implement the API functions*/
static int dac7731_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct dac7731_config *config = dev->config;
	uint8_t regval[2];
	int ret;
	
	/* Check if the channel is valid*/
	if (channel > 1) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}
	/* Writting any value to channel 1 will reset the dac output*/
	else if (channel == 1)
	{
		ret = dac7731_reset(dev);
		if(ret != 0)
		{
			LOG_ERR("Failed to reset the dac output");
			return ret;
		}
		/* successful */
		LOG_DBG("Reset the DAC");
		return 0;
	}
	

	/* Clip the input value to 16bits*/
	if (value > 0xFFFF)
	{
		LOG_WRN("Input value exceed maximum. Value Clipped.");
		value = 0xFFFF;
	}

	/* split the value into two bytes in big endian*/
	regval[0] = value >> 8;
	regval[1] = value & 0xFF;
	/* Send out the value through SPI*/
	const struct spi_buf buf[1] = { 
		{
			.buf = regval,
			.len = 2
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	if (k_is_in_isr()) {
		/* Prevent SPI transactions from an ISR */
		return -EWOULDBLOCK;
	}

	ret = spi_write_dt(&config->bus, &tx);
	if (ret != 0)
	{
		LOG_ERR("Failed to write to DAC7731");
		return ret;
	}
	/* load the value and update dac output*/
	ret = dac7731_load_value(dev);
	if (ret != 0)
	{
		LOG_ERR("Failed to load value to DAC7731");
		return ret;
	}
	/* success*/
	LOG_DBG("Set the DAC value");
	return 0;
}


/* Define API member*/
static const struct dac_driver_api dac7731_driver_api = {
	.write_value = dac7731_write_value
};

/* Init the device and gpio pins*/
static int dac7731_init(const struct device *dev)
{	
	int ret;
	LOG_DBG("Init start");

	/* Init the reset pin*/
	ret = dac7731_config_reset(dev);
	if(ret != 0){
		LOG_ERR("Fail to configure the reset pin");
		return ret;
	}

	/* Init the ldac pin*/
	ret = dac7731_config_ldac(dev);
	if(ret != 0){
		LOG_ERR("Fail to configure the ldac pin");
		return ret;
	}

	/* reset the dac chip*/
	ret = dac7731_reset(dev);
	if(ret != 0){
		LOG_ERR("Fail to reset the chip");
		return ret;
	}

	/* successful*/
	LOG_DBG("Init process done");
	return 0;
}

#define CREATE_DAC7731_INST(inst)								  \
	static struct dac7731_data dac7731_data_##inst = {};    \
	static const struct dac7731_config dac7731_config_##inst = {  \
		.reset_gpios = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),  \
		.ldac_gpios = GPIO_DT_SPEC_INST_GET(inst, ldac_gpios),	  \
		.bus = SPI_DT_SPEC_GET(DT_INST(inst, DT_DRV_COMPAT),					  \
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |				  \
			SPI_WORD_SET(8) | SPI_MODE_CPHA, 0) 				  \
	};										  					  \
	DEVICE_DT_INST_DEFINE(										  \
		inst, dac7731_init, 									  \
		NULL,													  \
		&dac7731_data_##inst,	  								  \
		&dac7731_config_##inst,   								  \
		POST_KERNEL,			  								  \
		CONFIG_DAC7731_INIT_PRIORITY, 							  \
		&dac7731_driver_api);

/* Call the device creation macro for each instance: */
DT_INST_FOREACH_STATUS_OKAY(CREATE_DAC7731_INST)
