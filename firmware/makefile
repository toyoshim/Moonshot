CFLAGS = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 \
	--code-size 0xec00 --stack-auto --Werror -Ichlib/src --opt-code-speed
CC     = sdcc
TARGET = moonshot
OBJS  = main.rel cdc_device.rel command.rel controller.rel settings.rel \
  ch559.rel flash.rel gpio.rel grove.rel hid.rel hid_dualshock3.rel \
	hid_guncon3.rel hid_keyboard.rel hid_mouse.rel hid_switch.rel hid_xbox.rel \
	i2c.rel led.rel serial.rel timer3.rel uart1.rel usb_device.rel usb_host.rel
DEFINES	= \
  -DSDA_BIT=P1_0 -DSDA_DIR=P1_DIR -DSDA_PU=P1_PU -DSDA_MASK="(1 << 0)" \
  -DSCL_BIT=P0_1 -DSCL_DIR=P0_DIR -DSCL_PU=P0_PU -DSCL_MASK="(1 << 1)"

.PHONY: all program run config data prod config build prod

all: build $(TARGET).bin $(TARGET)_proto1.bin

program: $(TARGET).bin
	ch559flasher -e -w $<

program_proto1: $(TARGET)_proto1.bin
	ch559flasher -e -w $<

run: program
	ch559flasher -b

config:
	ch559flasher -g 46

prod:
	ch559flasher -g 46 -E -w ../docs/firmwares/ms2_v1_00_0.bin -b

clean:
	rm -rf build $(TARGET).bin $(TARGET)_proto1.bin

.SILENT:
build:
	mkdir -p build

build/atari_proto1.rel: atari.c *.h chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -DPROTO1 -o $@ $<

build/%.rel: %.c *.h chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: chlib/src/%.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: chlib/src/usb/%.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: chlib/src/usb/hid/%.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/$(TARGET)_proto1.ihx: $(addprefix build/,$(OBJS)) build/atari_proto1.rel
	$(CC) $(CFLAGS) $^ -o $@

build/$(TARGET).ihx: $(addprefix build/,$(OBJS)) build/atari.rel
	$(CC) $(CFLAGS) $^ -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
