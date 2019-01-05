// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define CONCAT(a,b)     a##_##b
#define PORT(a,b)       CONCAT(a,b)

