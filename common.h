static inline int
is_power_of_two(int var)
{
	return var && !(var & (var-1));
}
