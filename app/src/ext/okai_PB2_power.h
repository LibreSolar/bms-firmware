
typedef unsigned int pb2_power_context_t;

#ifdef CONFIG_OKAI_PB2_POWER
int pb2_power_init();
pb2_power_context_t enable_pb2_power();
disable_pb2_power(pb2_power_context_t);
#else
int pb2_power_init();
static inline pb2_power_context_t enable_pb2_power() { return 0;}
static inline disable_pb2_power(pb2_power_context_t ) {};
#endif