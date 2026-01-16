#ifdef __cplusplus
extern "C"
{
#endif

void init_i2c_intr_gpio( uint8_t IRQpin );
bool is_i2c_intr_asserted();
void clear_i2c_intr_asserted();

#ifdef __cplusplus
}
#endif
