#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <libusb-1.0/libusb.h>

/*#define DEBUG*/

static const uint16_t RIVAL300_VID  = 0x1038;
static const uint16_t RIVAL300_PID  = 0x1710;

static const uint8_t SENS_PRESET1   = 0x01;
static const uint8_t SENS_PRESET2   = 0x02;
static const uint8_t LED_LOGO       = 0x01;
static const uint8_t LED_WHEEL      = 0x02;
static const uint16_t POLLRATE_125  = 0x0004;
static const uint16_t POLLRATE_250  = 0x0003;
static const uint16_t POLLRATE_500  = 0x0002;
static const uint16_t POLLRATE_1000 = 0x0001;

static const uint8_t CMD_SENS       = 0x03;
static const uint8_t CMD_POLLRATE   = 0x04;
static const uint8_t CMD_LED_EFFECT = 0x07;
static const uint8_t CMD_LED_COLOR  = 0x08;
static const uint8_t CMD_SAVE       = 0x09;

static void rival300_transmit_callback(struct libusb_transfer *t)
{
	free(t->buffer);
	libusb_free_transfer(t);
}

static int rival300_transmit(libusb_device_handle *devh,
	const uint8_t *data, int len)
{
	struct libusb_transfer *t;
	uint8_t *buffer = malloc(LIBUSB_CONTROL_SETUP_SIZE + len);
	uint8_t *payload = buffer + LIBUSB_CONTROL_SETUP_SIZE;

	memcpy(payload, data, len);

#ifdef DEBUG
	for (i = 0; i < len - 1; ++i)
		fprintf(stdout, "%02x ", data[i]);
	fprintf(stdout, "%02x\n", data[i]);
#endif

	libusb_fill_control_setup(buffer,
		LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
		0x09, 0x0200, 0, len);

	t = libusb_alloc_transfer(0);

	libusb_fill_control_transfer(t, devh,
		buffer, rival300_transmit_callback, NULL, 1000);

	if (libusb_submit_transfer(t) < 0) {
		fprintf(stderr, "%s: libusb_submit_transfer failed\n",
			__func__);

		free(buffer);
		libusb_free_transfer(t);
	}

	return libusb_handle_events(NULL);
}

static int rival300_sens(libusb_device_handle *devh,
                         uint8_t preset, uint16_t sensitivity)
{
	const uint16_t sens50 = sensitivity / 50;
	const uint8_t cmd[] = { CMD_SENS, preset, sens50 & 0xff, sens50 >> 8 };

	return rival300_transmit(devh, cmd, sizeof(cmd));
}

static int rival300_led_effect(libusb_device_handle *devh,
                               uint8_t led, uint8_t effect)
{
	const uint8_t cmd[] = { CMD_LED_EFFECT, led, effect };

	return rival300_transmit(devh, cmd, sizeof(cmd));
}

static int rival300_led_color(libusb_device_handle *devh,
                              uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
	const uint8_t cmd[] = { CMD_LED_COLOR, led, r, g, b };

	return rival300_transmit(devh, cmd, sizeof(cmd));
}

static int rival300_pollrate(libusb_device_handle *devh,
                             uint8_t pollrate)
{
	uint8_t cmd[] = { CMD_POLLRATE, 0x0, pollrate };

	return rival300_transmit(devh, cmd, sizeof(cmd));
}

static int rival300_save(libusb_device_handle *devh)
{
	const uint8_t cmd[] = { CMD_SAVE };

	return rival300_transmit(devh, cmd, sizeof(cmd));
}

/* comand line parsing */

struct arguments_t {
	int pollrate;
	int sens1;
	int sens2;
	int wheel_effect;
	int wheel_color[3];
	int logo_effect;
	int logo_color[3];
};

static int parse_args(int argc, char **argv, struct arguments_t *args)
{
	int args_count = 0;
	int flag;
	struct option longopts[] = {
		{ "pollrate",     required_argument, &flag, 1 },
		{ "sens1",        required_argument, &flag, 2 },
		{ "sens2",        required_argument, &flag, 3 },
		{ "wheel_effect", required_argument, &flag, 4 },
		{ "wheel_color",  required_argument, &flag, 5 },
		{ "logo_effect",  required_argument, &flag, 6 },
		{ "logo_color",   required_argument, &flag, 7 },
		{ 0 },
	};
	int longindex;

	while (getopt_long_only(argc, argv, "", longopts, &longindex) != -1) {
		switch(flag) {
		case 1:
			sscanf(optarg, "%d", &args->pollrate);
			break;
		case 2:
			sscanf(optarg, "%d", &args->sens1);
			break;
		case 3:
			sscanf(optarg, "%d", &args->sens2);
			break;
		case 4:
			sscanf(optarg, "%d", &args->wheel_effect);
			break;
		case 5:
			sscanf(optarg, "%d,%d,%d",
				&args->wheel_color[0],
				&args->wheel_color[1],
				&args->wheel_color[2]);
			break;
		case 6:
			sscanf(optarg, "%d", &args->logo_effect);
			break;
		case 7:
			sscanf(optarg, "%d,%d,%d",
				&args->logo_color[0],
				&args->logo_color[1],
				&args->logo_color[2]);
			break;
		default:
			return -1;
		}

		args_count++;
	}

	/* any command/option provided */
	return args_count > 0 ? 0 : 1;
}

static int execute_commands(libusb_device_handle *devh,
                            const struct arguments_t *args)
{
	int ret = -1;

	if (args->pollrate != -1)
		ret = rival300_pollrate(devh, args->pollrate);

	if (args->sens1 != -1)
		ret = rival300_sens(devh, SENS_PRESET1, args->sens1);

	if (args->sens2 != -1)
		ret = rival300_sens(devh, SENS_PRESET2, args->sens2);

	if (args->wheel_effect != -1)
		ret = rival300_led_effect(devh, LED_WHEEL, args->wheel_effect);

	if (args->logo_effect != -1)
		ret = rival300_led_effect(devh, LED_LOGO, args->logo_effect);

	if (args->wheel_color[0] != -1 &&
	    args->wheel_color[1] != -1 &&
	    args->wheel_color[2] != -1)
		ret = rival300_led_color(devh, LED_WHEEL,
			args->wheel_color[0],
			args->wheel_color[1],
			args->wheel_color[2]);

	if (args->logo_color[0] != -1 &&
	    args->logo_color[1] != -1 &&
	    args->logo_color[2] != -1)
		ret = rival300_led_color(devh, LED_LOGO,
			args->logo_color[0],
			args->logo_color[1],
			args->logo_color[2]);

	if (!ret)
		ret = rival300_save(devh);

	return ret;
}

static int usage(const char *cmd)
{
	fprintf(stdout, "usage: %s [OPTION=VALUE]...\n"
		"\n"
		"OPTION             VALUE\n"
		"  --pollrate       1 (1000 Hz)\n"
		"                   2 (500 Hz)\n"
		"                   3 (250 Hz)\n"
		"                   4 (125 Hz)\n"
		"  --sens1          50 - 6500 (step 50)\n"
		"  --sens2          50 - 6500 (step 50)\n"
		"  --wheel_effect   1 (static), 2 - 4 (pulse, slow->fast)\n"
		"  --logo_effect    1 (static), 2 - 4 (pulse, slow->fast)\n"
		"  --wheel_color    r,g,b (0-255,0-255,0-255)\n"
		"  --logo_color     r,g,b (0-255,0-255,0-255)\n",
		cmd);

	return 1;
}

int main(int argc, char **argv)
{
	struct arguments_t args = {
		-1,
		-1,
		-1,
		-1,
		{ -1, -1, -1 },
		-1,
		{ -1, -1, -1 },
	};
	libusb_device_handle *devh = NULL;

	if (parse_args(argc, argv, &args)) {
		fprintf(stderr, "invalid arguments\n");
		return usage(argv[0]);
	}

	libusb_init(NULL);
	libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_ERROR);

	devh = libusb_open_device_with_vid_pid(NULL,
	                                       RIVAL300_VID, RIVAL300_PID);
	if (devh == NULL) {
		fprintf(stderr, "failed to open device: %04x:%04x\n",
			RIVAL300_VID, RIVAL300_PID);
		return 2;
	}

	libusb_detach_kernel_driver(devh, 0);

	execute_commands(devh, &args);

	libusb_close(devh);
	libusb_exit(NULL);

	return 0;
}
