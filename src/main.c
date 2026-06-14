#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include <joybus/joybus.h>
#include <joybus/backend/rp2xxx.h>

// GPIO pin for Joybus data
#define JOYBUS_DATA_GPIO  12

// Settings for the external Joybus clock (SECCLK)
#define JOYBUS_CLOCK_GPIO 13
#define JOYBUS_CLOCK_HZ   1953125

// Maximum length of an input line, and maximum tokens per line
#define LINE_MAX          256
#define ARGV_MAX          16

// Joybus instance
static struct joybus_rp2xxx rp2xxx_bus;
static struct joybus *bus = JOYBUS(&rp2xxx_bus);

// Scratch buffer for transfer responses
static uint8_t response[JOYBUS_BLOCK_SIZE];

// Enable the external Joybus clock (SECCLK)
static void enable_joybus_clock(void)
{
  gpio_set_function(JOYBUS_CLOCK_GPIO, GPIO_FUNC_PWM);
  uint slice = pwm_gpio_to_slice_num(JOYBUS_CLOCK_GPIO);
  uint chan  = pwm_gpio_to_channel(JOYBUS_CLOCK_GPIO);

  // Configure PWM to generate a 1.953125 MHz clock
  // TODO: This assumes a 125 MHz system clock, need to generalize
  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg, 1);
  pwm_config_set_wrap(&cfg, 64 - 1);

  // Initialize PWM and set duty cycle to 50%
  pwm_init(slice, &cfg, true);
  pwm_set_chan_level(slice, chan, 32);
}

// Map a Joybus error code to a short token for the host
static const char *err_token(int result)
{
  switch (-result) {
    case JOYBUS_ERR_TIMEOUT:  return "timeout";
    case JOYBUS_ERR_BUSY:     return "busy";
    case JOYBUS_ERR_DISABLED: return "disabled";
    default:                  return "unknown";
  }
}

// Parse a 0-255 byte in hex or decimal. Returns 0 on success, -1 on bad input.
static int parse_byte(const char *s, uint8_t *out)
{
  if (*s == '-' || *s == '+')
    return -1;

  char *end;
  unsigned long v = strtoul(s, &end, 0);
  if (end == s || *end != '\0' || v > 0xFF)
    return -1;

  *out = (uint8_t)v;
  return 0;
}

// Read one '\n'-terminated line into buf (no echo). Returns length, or -1 on EOF.
static int read_line(char *buf, int max)
{
  int len = 0;
  while (len < max - 1) {
    int c = getchar();
    if (c == PICO_ERROR_TIMEOUT || c == EOF)
      return -1;
    if (c == '\r')
      continue;
    if (c == '\n')
      break;
    buf[len++] = (char)c;
  }
  buf[len] = '\0';
  return len;
}

// Handle one "transfer <response_len> <command> [args...]" request
static void do_transfer(int argc, char **argv)
{
  // Parse the expected response length
  uint8_t response_len;
  if (argc < 3 || parse_byte(argv[1], &response_len) < 0) {
    printf("!! badarg\n");
    return;
  }

  // Parse each remaining argument as a data byte to write
  int len = argc - 2;
  for (int i = 0; i < len; i++) {
    if (parse_byte(argv[i + 2], &bus->command_buffer[i]) < 0) {
      printf("!! badarg\n");
      return;
    }
  }

  // Send the command
  int result = joybus_transfer_sync(bus, bus->command_buffer, (uint8_t)len, response, response_len);
  if (result < 0) {
    printf("!! %s\n", err_token(result));
    return;
  }

  // Print the response bytes
  printf("RX");
  for (int i = 0; i < result; i++)
    printf(" %02X", response[i]);
  printf("\n");
}

int main()
{
  // Initialize stdio
  stdio_init_all();

  // Enable the external Joybus clock (SECCLK)
  enable_joybus_clock();

  // Initialize Joybus
  joybus_rp2xxx_init(&rp2xxx_bus, JOYBUS_DATA_GPIO, pio0);
  joybus_enable(bus);

  // Read lines from stdin and dispatch commands
  char line[LINE_MAX];
  while (true) {
    if (read_line(line, sizeof line) < 0)
      continue;

    // Tokenize the line in place on whitespace
    int argc = 0;
    char *argv[ARGV_MAX];
    for (char *t = strtok(line, " \t"); t != NULL && argc < ARGV_MAX; t = strtok(NULL, " \t"))
      argv[argc++] = t;

    // Ignore empty lines
    if (argc == 0)
      continue;

    // Only handle "transfer" commands for now
    if (strcmp(argv[0], "transfer") == 0) {
      do_transfer(argc, argv);
    } else {
      printf("!! badcmd\n");
    }

    fflush(stdout);
  }
}
