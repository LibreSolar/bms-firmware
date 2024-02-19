#if CONFIG_OKAI_PB2_POWER
#include <zephyr.h>
#include <sys/onoff.h>
#include <drivers/gpio.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(regulator_fixed, CONFIG_REGULATOR_LOG_LEVEL);

pb2_power_context_t enable_pb2_power()
{
    struct onoff_client cli;
    
}

disable_pb2_power(pb2_power_context_t){}



#endif