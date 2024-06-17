# Out of tree Texas Instruments DAC7731 driver module for Zephyr RTOS
*The DAC7731 is a 16-bit Digital-to-Analog Converter (DAC) which provides 16 bits of monotonic performance over the specified operating temperature range and offers a +10V internal reference.*

## Hardware Configuration
The DAC7731 can be configured through different pin setting. Options as below:
- DAC output range: \
The chip can be configured in a ±10V, ±5V, or +10V output range. 

- Reference voltage: \
The chip has +10V internal voltage, which can be enabled or disabled. It can also use an external voltage source as reference.

- Reset behavior: \
The DAC output can be reset to either min-scale or mid-scale.



This driver module doesn't provide any capibility to change the configurations above. Please refer to the datasheet for further information if needed. The driver only consider the following pins:
- RST pin
- SPI pins
- LDAC pin

## Supported Zephyr versions
* 3.6.99 (May 2024)
## Usage
### Module installation
Add this project to your `west.yml` manifest:
```yaml
 # out of tree device driver
	- name: DAC7731
      path: modules/dac7731
      revision: refs/tags/zephyr-v3.6.99
      url: https://github.com/TinNotTim/dac7731_zephyr_driver
```

So your projects should look something like this:
```yaml
manifest:
  projects:
    - name: zephyr
      url: https://github.com/zephyrproject-rtos/zephyr
      revision: refs/tags/zephyr-v3.2.0
      import: true
# out of tree device driver
    - name: DAC7731
      path: modules/dac7731
      revision: refs/tags/zephyr-v3.6.99
      url: https://github.com/TinNotTim/dac7731_zephyr_driver
```

This will import the driver and allow you to use it in your code.

Additionally make sure that you run `west update` when you've added this entry to your `west.yml`.

### Driver configuration
Enable sensor driver subsystem and DAC7731 driver by adding these entries to your `prj.conf`:
```ini
CONFIG_DAC=y
CONFIG_DAC7731=y
```

The DAC7731 can be configured through different pin setting. Options as below:
- DAC output range: \
The chip can be configured in a ±10V, ±5V, or +10V output range. 

- Reference voltage: \
The chip has +10V internal voltage, which can be enabled or disabled. It can also use an external voltage source as reference.

- Reset behavior: \
The DAC output can be reset to either min-scale or mid-scale.



This driver module doesn't provide any capibility to change the configurations above. Please refer to the datasheet for further information if needed. The driver only consider the following pins:
- RST pin
- SPI pins
- LDAC pin

Define DAC7731 in your board `.overlay` like this example:
```dts
&spi3 {
    status = "okay";
    pinctrl-0 = <&spi3_sck_pb3 &spi3_miso_pb4 &spi3_mosi_pb5>;
	pinctrl-names = "default";
	cs-gpios = <&gpiod 12 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>

    dac7731_1: dac7731@0 {
        compatible = "ti,dac7731";
        spi-max-frequency = <2000000>;
        reg = <0>;
        reset-gpios = <&gpiod 11 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        ldac-gpios = <&gpiod 13 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
    };
};
```

### Driver usage
```c
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

static K_THREAD_STACK_DEFINE(dac1_thread_stack, 512);
static struct k_thread dac1_thread_data;

/* Declare the membership in registered logging module*/
LOG_MODULE_REGISTER(DAC1_TEST, CONFIG_SPI_LOG_LEVEL);

void dac1_thread(void)
{
    /* Get the device pointer to dac7731 */
	const struct device *const dac7731 = DEVICE_DT_GET_ONE(ti_dac7731);

	if (!device_is_ready(dac7731)) {
		LOG_ERR("DAC: device not ready.");
		return;
	}
	
	while (IS_ENABLED(CONFIG_DAC7731)) {      
		/* Write a 16-bit value to channel-0 to set the dac output */
		/* The driver will clip the value to 0xFFFF*/ 
		dac_write_value(dac7731, 0, 0x8000+0xFF);
		k_sleep(K_MSEC(1000));

		/* Write an arbitrary value to channel-1 to reset the dac output */
		/* The DAC API doesn't provide a dedicated reset function*/
		dac_write_value(dac7731, 1, 0);
		k_sleep(K_MSEC(1000));
	}
	k_sleep(K_FOREVER);
}


int main(void){
	
	k_thread_create(&dac1_thread_data, dac1_thread_stack,
					K_THREAD_STACK_SIZEOF(dac1_thread_stack),
					(k_thread_entry_t)dac1_thread,
					NULL, NULL, NULL,
					K_PRIO_COOP(7), 0,
					K_NO_WAIT);
	k_thread_name_set(&dac1_thread_data, "dac1_print");

	while(1){
		k_sleep(K_SECONDS(10));
	}

	return 0;
}
```
Relevant `prj.conf`:
```ini
CONFIG_DAC=y
CONFIG_DAC7731=y
CONFIG_LOG=y
```