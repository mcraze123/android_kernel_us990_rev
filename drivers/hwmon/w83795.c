/*
 *  w83795.c - Linux kernel driver for hardware monitoring
 *  Copyright (C) 2008 Nuvoton Technology Corp.
 *                Wei Song
 *  Copyright (C) 2010 Jean Delvare <khali@linux-fr.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation - version 2.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA.
 *
 *  Supports following chips:
 *
 *  Chip       #vin   #fanin #pwm #temp #dts wchipid  vendid  i2c  ISA
 *  w83795g     21     14     8     6     8    0x79   0x5ca3  yes   no
 *  w83795adg   18     14     2     6     8    0x79   0x5ca3  yes   no
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>

/*                   */
static const unsigned short normal_i2c[] = {
	0x2c, 0x2d, 0x2e, 0x2f, I2C_CLIENT_END
};


static bool reset;
module_param(reset, bool, 0);
MODULE_PARM_DESC(reset, "Set to 1 to reset chip, not recommended");


#define W83795_REG_BANKSEL		0x00
#define W83795_REG_VENDORID		0xfd
#define W83795_REG_CHIPID		0xfe
#define W83795_REG_DEVICEID		0xfb
#define W83795_REG_DEVICEID_A		0xff

#define W83795_REG_I2C_ADDR		0xfc
#define W83795_REG_CONFIG		0x01
#define W83795_REG_CONFIG_CONFIG48	0x04
#define W83795_REG_CONFIG_START	0x01

/*                                   */
#define W83795_REG_VOLT_CTRL1		0x02
#define W83795_REG_VOLT_CTRL2		0x03
#define W83795_REG_TEMP_CTRL1		0x04
#define W83795_REG_TEMP_CTRL2		0x05
#define W83795_REG_FANIN_CTRL1		0x06
#define W83795_REG_FANIN_CTRL2		0x07
#define W83795_REG_VMIGB_CTRL		0x08

#define TEMP_READ			0
#define TEMP_CRIT			1
#define TEMP_CRIT_HYST			2
#define TEMP_WARN			3
#define TEMP_WARN_HYST			4
/*
                                                        
                                        
 */
static const u16 W83795_REG_TEMP[][5] = {
	{0x21, 0x96, 0x97, 0x98, 0x99},	/*         */
	{0x22, 0x9a, 0x9b, 0x9c, 0x9d},	/*         */
	{0x23, 0x9e, 0x9f, 0xa0, 0xa1},	/*         */
	{0x24, 0xa2, 0xa3, 0xa4, 0xa5},	/*         */
	{0x1f, 0xa6, 0xa7, 0xa8, 0xa9},	/*     */
	{0x20, 0xaa, 0xab, 0xac, 0xad},	/*     */
};

#define IN_READ				0
#define IN_MAX				1
#define IN_LOW				2
static const u16 W83795_REG_IN[][3] = {
	/*                 */
	{0x10, 0x70, 0x71},	/*       */
	{0x11, 0x72, 0x73},	/*       */
	{0x12, 0x74, 0x75},	/*       */
	{0x13, 0x76, 0x77},	/*       */
	{0x14, 0x78, 0x79},	/*       */
	{0x15, 0x7a, 0x7b},	/*       */
	{0x16, 0x7c, 0x7d},	/*       */
	{0x17, 0x7e, 0x7f},	/*       */
	{0x18, 0x80, 0x81},	/*       */
	{0x19, 0x82, 0x83},	/*        */
	{0x1A, 0x84, 0x85},	/*        */
	{0x1B, 0x86, 0x87},	/*     */
	{0x1C, 0x88, 0x89},	/*      */
	{0x1D, 0x8a, 0x8b},	/*      */
	{0x1E, 0x8c, 0x8d},	/*      */
	{0x1F, 0xa6, 0xa7},	/*        */
	{0x20, 0xaa, 0xab},	/*        */
	{0x21, 0x96, 0x97},	/*        */
	{0x22, 0x9a, 0x9b},	/*        */
	{0x23, 0x9e, 0x9f},	/*        */
	{0x24, 0xa2, 0xa3},	/*        */
};
#define W83795_REG_VRLSB		0x3C

static const u8 W83795_REG_IN_HL_LSB[] = {
	0x8e,	/*         */
	0x90,	/*         */
	0x92,	/*          */
	0x94,	/*                        */
	0xa8,	/*        */
	0xac,	/*        */
	0x98,	/*        */
	0x9c,	/*        */
	0xa0,	/*        */
	0xa4,	/*        */
};

#define IN_LSB_REG(index, type) \
	(((type) == 1) ? W83795_REG_IN_HL_LSB[(index)] \
	: (W83795_REG_IN_HL_LSB[(index)] + 1))

#define IN_LSB_SHIFT			0
#define IN_LSB_IDX			1
static const u8 IN_LSB_SHIFT_IDX[][2] = {
	/*                             */
	{0x00, 0x00},	/*       */
	{0x02, 0x00},	/*       */
	{0x04, 0x00},	/*       */
	{0x06, 0x00},	/*       */
	{0x00, 0x01},	/*       */
	{0x02, 0x01},	/*       */
	{0x04, 0x01},	/*       */
	{0x06, 0x01},	/*       */
	{0x00, 0x02},	/*       */
	{0x02, 0x02},	/*        */
	{0x04, 0x02},	/*        */
	{0x00, 0x03},	/*     */
	{0x02, 0x03},	/*      */
	{0x04, 0x03},	/*      */
	{0x06, 0x03},	/*      */
	{0x06, 0x04},	/*        */
	{0x06, 0x05},	/*        */
	{0x06, 0x06},	/*        */
	{0x06, 0x07},	/*        */
	{0x06, 0x08},	/*        */
	{0x06, 0x09},	/*        */
};


#define W83795_REG_FAN(index)		(0x2E + (index))
#define W83795_REG_FAN_MIN_HL(index)	(0xB6 + (index))
#define W83795_REG_FAN_MIN_LSB(index)	(0xC4 + (index) / 2)
#define W83795_REG_FAN_MIN_LSB_SHIFT(index) \
	(((index) & 1) ? 4 : 0)

#define W83795_REG_VID_CTRL		0x6A

#define W83795_REG_ALARM_CTRL		0x40
#define ALARM_CTRL_RTSACS		(1 << 7)
#define W83795_REG_ALARM(index)		(0x41 + (index))
#define W83795_REG_CLR_CHASSIS		0x4D
#define W83795_REG_BEEP(index)		(0x50 + (index))

#define W83795_REG_OVT_CFG		0x58
#define OVT_CFG_SEL			(1 << 7)


#define W83795_REG_FCMS1		0x201
#define W83795_REG_FCMS2		0x208
#define W83795_REG_TFMR(index)		(0x202 + (index))
#define W83795_REG_FOMC			0x20F

#define W83795_REG_TSS(index)		(0x209 + (index))

#define TSS_MAP_RESERVED		0xff
static const u8 tss_map[4][6] = {
	{ 0,  1,  2,  3,  4,  5},
	{ 6,  7,  8,  9,  0,  1},
	{10, 11, 12, 13,  2,  3},
	{ 4,  5,  4,  5, TSS_MAP_RESERVED, TSS_MAP_RESERVED},
};

#define PWM_OUTPUT			0
#define PWM_FREQ			1
#define PWM_START			2
#define PWM_NONSTOP			3
#define PWM_STOP_TIME			4
#define W83795_REG_PWM(index, nr)	(0x210 + (nr) * 8 + (index))

#define W83795_REG_FTSH(index)		(0x240 + (index) * 2)
#define W83795_REG_FTSL(index)		(0x241 + (index) * 2)
#define W83795_REG_TFTS			0x250

#define TEMP_PWM_TTTI			0
#define TEMP_PWM_CTFS			1
#define TEMP_PWM_HCT			2
#define TEMP_PWM_HOT			3
#define W83795_REG_TTTI(index)		(0x260 + (index))
#define W83795_REG_CTFS(index)		(0x268 + (index))
#define W83795_REG_HT(index)		(0x270 + (index))

#define SF4_TEMP			0
#define SF4_PWM				1
#define W83795_REG_SF4_TEMP(temp_num, index) \
	(0x280 + 0x10 * (temp_num) + (index))
#define W83795_REG_SF4_PWM(temp_num, index) \
	(0x288 + 0x10 * (temp_num) + (index))

#define W83795_REG_DTSC			0x301
#define W83795_REG_DTSE			0x302
#define W83795_REG_DTS(index)		(0x26 + (index))
#define W83795_REG_PECI_TBASE(index)	(0x320 + (index))

#define DTS_CRIT			0
#define DTS_CRIT_HYST			1
#define DTS_WARN			2
#define DTS_WARN_HYST			3
#define W83795_REG_DTS_EXT(index)	(0xB2 + (index))

#define SETUP_PWM_DEFAULT		0
#define SETUP_PWM_UPTIME		1
#define SETUP_PWM_DOWNTIME		2
#define W83795_REG_SETUP_PWM(index)    (0x20C + (index))

static inline u16 in_from_reg(u8 index, u16 val)
{
	/*                                                       */
	if (index >= 12 && index <= 14)
		return val * 6;
	else
		return val * 2;
}

static inline u16 in_to_reg(u8 index, u16 val)
{
	if (index >= 12 && index <= 14)
		return val / 6;
	else
		return val / 2;
}

static inline unsigned long fan_from_reg(u16 val)
{
	if ((val == 0xfff) || (val == 0))
		return 0;
	return 1350000UL / val;
}

static inline u16 fan_to_reg(long rpm)
{
	if (rpm <= 0)
		return 0x0fff;
	return SENSORS_LIMIT((1350000 + (rpm >> 1)) / rpm, 1, 0xffe);
}

static inline unsigned long time_from_reg(u8 reg)
{
	return reg * 100;
}

static inline u8 time_to_reg(unsigned long val)
{
	return SENSORS_LIMIT((val + 50) / 100, 0, 0xff);
}

static inline long temp_from_reg(s8 reg)
{
	return reg * 1000;
}

static inline s8 temp_to_reg(long val, s8 min, s8 max)
{
	return SENSORS_LIMIT(val / 1000, min, max);
}

static const u16 pwm_freq_cksel0[16] = {
	1024, 512, 341, 256, 205, 171, 146, 128,
	85, 64, 32, 16, 8, 4, 2, 1
};

static unsigned int pwm_freq_from_reg(u8 reg, u16 clkin)
{
	unsigned long base_clock;

	if (reg & 0x80) {
		base_clock = clkin * 1000 / ((clkin == 48000) ? 384 : 256);
		return base_clock / ((reg & 0x7f) + 1);
	} else
		return pwm_freq_cksel0[reg & 0x0f];
}

static u8 pwm_freq_to_reg(unsigned long val, u16 clkin)
{
	unsigned long base_clock;
	u8 reg0, reg1;
	unsigned long best0, best1;

	/*                        */
	for (reg0 = 0; reg0 < ARRAY_SIZE(pwm_freq_cksel0) - 1; reg0++) {
		if (val > (pwm_freq_cksel0[reg0] +
			   pwm_freq_cksel0[reg0 + 1]) / 2)
			break;
	}
	if (val < 375)	/*                           */
		return reg0;
	best0 = pwm_freq_cksel0[reg0];

	/*                        */
	base_clock = clkin * 1000 / ((clkin == 48000) ? 384 : 256);
	reg1 = SENSORS_LIMIT(DIV_ROUND_CLOSEST(base_clock, val), 1, 128);
	best1 = base_clock / reg1;
	reg1 = 0x80 | (reg1 - 1);

	/*                        */
	if (abs(val - best0) > abs(val - best1))
		return reg1;
	else
		return reg0;
}

enum chip_types {w83795g, w83795adg};

struct w83795_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	unsigned long last_updated;	/*            */
	enum chip_types chip_type;

	u8 bank;

	u32 has_in;		/*                           */
	u8 has_dyn_in;		/*                          */
	u16 in[21][3];		/*                               */
	u8 in_lsb[10][3];	/*                              */
	u8 has_gain;		/*                       */

	u16 has_fan;		/*                       */
	u16 fan[14];		/*                        */
	u16 fan_min[14];	/*                        */

	u8 has_temp;		/*                               */
	s8 temp[6][5];		/*                                           */
	u8 temp_read_vrlsb[6];
	u8 temp_mode;		/*                            */
	u8 temp_src[3];		/*                */

	u8 enable_dts;		/*
                              
                                    
                                          
     */
	u8 has_dts;		/*                         */
	s8 dts[8];		/*                */
	u8 dts_read_vrlsb[8];	/*                */
	s8 dts_ext[4];		/*                */

	u8 has_pwm;		/*
                                                   
                                                
           
     */
	u8 pwm[8][5];		/*
                                           
                           
     */
	u16 clkin;		/*                        */
	u8 pwm_fcms[2];		/*                */
	u8 pwm_tfmr[6];		/*                */
	u8 pwm_fomc;		/*                */

	u16 target_speed[8];	/*
                                             
             
     */
	u8 tol_speed;		/*                           */
	u8 pwm_temp[6][4];	/*                      */
	u8 sf4_reg[6][2][7];	/*                                 */

	u8 setup_pwm[3];	/*                */

	u8 alarms[6];		/*                */
	u8 enable_beep;
	u8 beeps[6];		/*                */

	char valid;
	char valid_limits;
	char valid_pwm_config;
};

/*
                  
                                                                 
 */

/*                                                                          */
static int w83795_set_bank(struct i2c_client *client, u8 bank)
{
	struct w83795_data *data = i2c_get_clientdata(client);
	int err;

	/*                                                */
	if ((data->bank & 0x07) == bank)
		return 0;

	/*                                             */
	bank |= data->bank & ~0x07;
	err = i2c_smbus_write_byte_data(client, W83795_REG_BANKSEL, bank);
	if (err < 0) {
		dev_err(&client->dev,
			"Failed to set bank to %d, err %d\n",
			(int)bank, err);
		return err;
	}
	data->bank = bank;

	return 0;
}

/*                                                                          */
static u8 w83795_read(struct i2c_client *client, u16 reg)
{
	int err;

	err = w83795_set_bank(client, reg >> 8);
	if (err < 0)
		return 0x00;	/*           */

	err = i2c_smbus_read_byte_data(client, reg & 0xff);
	if (err < 0) {
		dev_err(&client->dev,
			"Failed to read from register 0x%03x, err %d\n",
			(int)reg, err);
		return 0x00;	/*           */
	}
	return err;
}

/*                                                                          */
static int w83795_write(struct i2c_client *client, u16 reg, u8 value)
{
	int err;

	err = w83795_set_bank(client, reg >> 8);
	if (err < 0)
		return err;

	err = i2c_smbus_write_byte_data(client, reg & 0xff, value);
	if (err < 0)
		dev_err(&client->dev,
			"Failed to write to register 0x%03x, err %d\n",
			(int)reg, err);
	return err;
}

static void w83795_update_limits(struct i2c_client *client)
{
	struct w83795_data *data = i2c_get_clientdata(client);
	int i, limit;
	u8 lsb;

	/*                         */
	for (i = 0; i < ARRAY_SIZE(data->in); i++) {
		if (!(data->has_in & (1 << i)))
			continue;
		data->in[i][IN_MAX] =
			w83795_read(client, W83795_REG_IN[i][IN_MAX]);
		data->in[i][IN_LOW] =
			w83795_read(client, W83795_REG_IN[i][IN_LOW]);
	}
	for (i = 0; i < ARRAY_SIZE(data->in_lsb); i++) {
		if ((i == 2 && data->chip_type == w83795adg) ||
		    (i >= 4 && !(data->has_in & (1 << (i + 11)))))
			continue;
		data->in_lsb[i][IN_MAX] =
			w83795_read(client, IN_LSB_REG(i, IN_MAX));
		data->in_lsb[i][IN_LOW] =
			w83795_read(client, IN_LSB_REG(i, IN_LOW));
	}

	/*                     */
	lsb = 0; /*                          */
	for (i = 0; i < ARRAY_SIZE(data->fan); i++) {
		/*
                                                          
                                   
   */
		if ((i & 1) == 0 && (data->has_fan & (3 << i)))
			lsb = w83795_read(client, W83795_REG_FAN_MIN_LSB(i));

		if (!(data->has_fan & (1 << i)))
			continue;
		data->fan_min[i] =
			w83795_read(client, W83795_REG_FAN_MIN_HL(i)) << 4;
		data->fan_min[i] |=
			(lsb >> W83795_REG_FAN_MIN_LSB_SHIFT(i)) & 0x0F;
	}

	/*                             */
	for (i = 0; i < ARRAY_SIZE(data->temp); i++) {
		if (!(data->has_temp & (1 << i)))
			continue;
		for (limit = TEMP_CRIT; limit <= TEMP_WARN_HYST; limit++)
			data->temp[i][limit] =
				w83795_read(client, W83795_REG_TEMP[i][limit]);
	}

	/*                     */
	if (data->enable_dts) {
		for (limit = DTS_CRIT; limit <= DTS_WARN_HYST; limit++)
			data->dts_ext[limit] =
				w83795_read(client, W83795_REG_DTS_EXT(limit));
	}

	/*                    */
	if (data->enable_beep) {
		for (i = 0; i < ARRAY_SIZE(data->beeps); i++)
			data->beeps[i] =
				w83795_read(client, W83795_REG_BEEP(i));
	}

	data->valid_limits = 1;
}

static struct w83795_data *w83795_update_pwm_config(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	int i, tmp;

	mutex_lock(&data->update_lock);

	if (data->valid_pwm_config)
		goto END;

	/*                                   */
	for (i = 0; i < ARRAY_SIZE(data->temp_src); i++)
		data->temp_src[i] = w83795_read(client, W83795_REG_TSS(i));

	/*                                           */
	data->pwm_fcms[0] = w83795_read(client, W83795_REG_FCMS1);
	data->pwm_fcms[1] = w83795_read(client, W83795_REG_FCMS2);
	for (i = 0; i < ARRAY_SIZE(data->pwm_tfmr); i++)
		data->pwm_tfmr[i] = w83795_read(client, W83795_REG_TFMR(i));
	data->pwm_fomc = w83795_read(client, W83795_REG_FOMC);
	for (i = 0; i < data->has_pwm; i++) {
		for (tmp = PWM_FREQ; tmp <= PWM_STOP_TIME; tmp++)
			data->pwm[i][tmp] =
				w83795_read(client, W83795_REG_PWM(i, tmp));
	}
	for (i = 0; i < ARRAY_SIZE(data->target_speed); i++) {
		data->target_speed[i] =
			w83795_read(client, W83795_REG_FTSH(i)) << 4;
		data->target_speed[i] |=
			w83795_read(client, W83795_REG_FTSL(i)) >> 4;
	}
	data->tol_speed = w83795_read(client, W83795_REG_TFTS) & 0x3f;

	for (i = 0; i < ARRAY_SIZE(data->pwm_temp); i++) {
		data->pwm_temp[i][TEMP_PWM_TTTI] =
			w83795_read(client, W83795_REG_TTTI(i)) & 0x7f;
		data->pwm_temp[i][TEMP_PWM_CTFS] =
			w83795_read(client, W83795_REG_CTFS(i));
		tmp = w83795_read(client, W83795_REG_HT(i));
		data->pwm_temp[i][TEMP_PWM_HCT] = tmp >> 4;
		data->pwm_temp[i][TEMP_PWM_HOT] = tmp & 0x0f;
	}

	/*                             */
	for (i = 0; i < ARRAY_SIZE(data->sf4_reg); i++) {
		for (tmp = 0; tmp < 7; tmp++) {
			data->sf4_reg[i][SF4_TEMP][tmp] =
				w83795_read(client,
					    W83795_REG_SF4_TEMP(i, tmp));
			data->sf4_reg[i][SF4_PWM][tmp] =
				w83795_read(client, W83795_REG_SF4_PWM(i, tmp));
		}
	}

	/*                */
	for (i = 0; i < ARRAY_SIZE(data->setup_pwm); i++)
		data->setup_pwm[i] =
			w83795_read(client, W83795_REG_SETUP_PWM(i));

	data->valid_pwm_config = 1;

END:
	mutex_unlock(&data->update_lock);
	return data;
}

static struct w83795_data *w83795_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	u16 tmp;
	u8 intrusion;
	int i;

	mutex_lock(&data->update_lock);

	if (!data->valid_limits)
		w83795_update_limits(client);

	if (!(time_after(jiffies, data->last_updated + HZ * 2)
	      || !data->valid))
		goto END;

	/*                           */
	for (i = 0; i < ARRAY_SIZE(data->in); i++) {
		if (!(data->has_in & (1 << i)))
			continue;
		tmp = w83795_read(client, W83795_REG_IN[i][IN_READ]) << 2;
		tmp |= w83795_read(client, W83795_REG_VRLSB) >> 6;
		data->in[i][IN_READ] = tmp;
	}

	/*                                              */
	if (data->has_dyn_in) {
		u8 lsb_max = w83795_read(client, IN_LSB_REG(0, IN_MAX));
		u8 lsb_low = w83795_read(client, IN_LSB_REG(0, IN_LOW));

		for (i = 0; i < 3; i++) {
			if (!(data->has_dyn_in & (1 << i)))
				continue;
			data->in[i][IN_MAX] =
				w83795_read(client, W83795_REG_IN[i][IN_MAX]);
			data->in[i][IN_LOW] =
				w83795_read(client, W83795_REG_IN[i][IN_LOW]);
			data->in_lsb[i][IN_MAX] = (lsb_max >> (2 * i)) & 0x03;
			data->in_lsb[i][IN_LOW] = (lsb_low >> (2 * i)) & 0x03;
		}
	}

	/*            */
	for (i = 0; i < ARRAY_SIZE(data->fan); i++) {
		if (!(data->has_fan & (1 << i)))
			continue;
		data->fan[i] = w83795_read(client, W83795_REG_FAN(i)) << 4;
		data->fan[i] |= w83795_read(client, W83795_REG_VRLSB) >> 4;
	}

	/*                    */
	for (i = 0; i < ARRAY_SIZE(data->temp); i++) {
		data->temp[i][TEMP_READ] =
			w83795_read(client, W83795_REG_TEMP[i][TEMP_READ]);
		data->temp_read_vrlsb[i] =
			w83795_read(client, W83795_REG_VRLSB);
	}

	/*                        */
	if (data->enable_dts) {
		for (i = 0; i < ARRAY_SIZE(data->dts); i++) {
			if (!(data->has_dts & (1 << i)))
				continue;
			data->dts[i] =
				w83795_read(client, W83795_REG_DTS(i));
			data->dts_read_vrlsb[i] =
				w83795_read(client, W83795_REG_VRLSB);
		}
	}

	/*                   */
	for (i = 0; i < data->has_pwm; i++) {
		data->pwm[i][PWM_OUTPUT] =
		    w83795_read(client, W83795_REG_PWM(i, PWM_OUTPUT));
	}

	/*
                               
                                                                 
                                                              
  */
	tmp = w83795_read(client, W83795_REG_ALARM_CTRL);
	/*                                                    */
	if (tmp & ALARM_CTRL_RTSACS)
		w83795_write(client, W83795_REG_ALARM_CTRL,
			     tmp & ~ALARM_CTRL_RTSACS);
	intrusion = w83795_read(client, W83795_REG_ALARM(5)) & (1 << 6);
	/*                            */
	w83795_write(client, W83795_REG_ALARM_CTRL, tmp | ALARM_CTRL_RTSACS);
	for (i = 0; i < ARRAY_SIZE(data->alarms); i++)
		data->alarms[i] = w83795_read(client, W83795_REG_ALARM(i));
	data->alarms[5] |= intrusion;
	/*                                          */
	if (!(tmp & ALARM_CTRL_RTSACS))
		w83795_write(client, W83795_REG_ALARM_CTRL,
			     tmp & ~ALARM_CTRL_RTSACS);

	data->last_updated = jiffies;
	data->valid = 1;

END:
	mutex_unlock(&data->update_lock);
	return data;
}

/*
                   
 */

#define ALARM_STATUS      0
#define BEEP_ENABLE       1
static ssize_t
show_alarm_beep(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = w83795_update_device(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index >> 3;
	int bit = sensor_attr->index & 0x07;
	u8 val;

	if (nr == ALARM_STATUS)
		val = (data->alarms[index] >> bit) & 1;
	else		/*             */
		val = (data->beeps[index] >> bit) & 1;

	return sprintf(buf, "%u\n", val);
}

static ssize_t
store_beep(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index >> 3;
	int shift = sensor_attr->index & 0x07;
	u8 beep_bit = 1 << shift;
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;
	if (val != 0 && val != 1)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->beeps[index] = w83795_read(client, W83795_REG_BEEP(index));
	data->beeps[index] &= ~beep_bit;
	data->beeps[index] |= val << shift;
	w83795_write(client, W83795_REG_BEEP(index), data->beeps[index]);
	mutex_unlock(&data->update_lock);

	return count;
}

/*                                */
static ssize_t
store_chassis_clear(struct device *dev,
		    struct device_attribute *attr, const char *buf,
		    size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0 || val != 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	val = w83795_read(client, W83795_REG_CLR_CHASSIS);
	val |= 0x80;
	w83795_write(client, W83795_REG_CLR_CHASSIS, val);

	/*                                      */
	w83795_read(client, W83795_REG_ALARM(5));
	data->valid = 0;
	mutex_unlock(&data->update_lock);
	return count;
}

#define FAN_INPUT     0
#define FAN_MIN       1
static ssize_t
show_fan(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	struct w83795_data *data = w83795_update_device(dev);
	u16 val;

	if (nr == FAN_INPUT)
		val = data->fan[index] & 0x0fff;
	else
		val = data->fan_min[index] & 0x0fff;

	return sprintf(buf, "%lu\n", fan_from_reg(val));
}

static ssize_t
store_fan_min(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index;
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	val = fan_to_reg(val);

	mutex_lock(&data->update_lock);
	data->fan_min[index] = val;
	w83795_write(client, W83795_REG_FAN_MIN_HL(index), (val >> 4) & 0xff);
	val &= 0x0f;
	if (index & 1) {
		val <<= 4;
		val |= w83795_read(client, W83795_REG_FAN_MIN_LSB(index))
		       & 0x0f;
	} else {
		val |= w83795_read(client, W83795_REG_FAN_MIN_LSB(index))
		       & 0xf0;
	}
	w83795_write(client, W83795_REG_FAN_MIN_LSB(index), val & 0xff);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t
show_pwm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data;
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned int val;

	data = nr == PWM_OUTPUT ? w83795_update_device(dev)
				: w83795_update_pwm_config(dev);

	switch (nr) {
	case PWM_STOP_TIME:
		val = time_from_reg(data->pwm[index][nr]);
		break;
	case PWM_FREQ:
		val = pwm_freq_from_reg(data->pwm[index][nr], data->clkin);
		break;
	default:
		val = data->pwm[index][nr];
		break;
	}

	return sprintf(buf, "%u\n", val);
}

static ssize_t
store_pwm(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	switch (nr) {
	case PWM_STOP_TIME:
		val = time_to_reg(val);
		break;
	case PWM_FREQ:
		val = pwm_freq_to_reg(val, data->clkin);
		break;
	default:
		val = SENSORS_LIMIT(val, 0, 0xff);
		break;
	}
	w83795_write(client, W83795_REG_PWM(index, nr), val);
	data->pwm[index][nr] = val;
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_pwm_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	struct w83795_data *data = w83795_update_pwm_config(dev);
	int index = sensor_attr->index;
	u8 tmp;

	/*                   */
	if (data->pwm_fcms[0] & (1 << index)) {
		tmp = 2;
		goto out;
	}
	/*                                    */
	for (tmp = 0; tmp < 6; tmp++) {
		if (data->pwm_tfmr[tmp] & (1 << index)) {
			tmp = 3;
			goto out;
		}
	}
	/*             */
	tmp = 1;

out:
	return sprintf(buf, "%u\n", tmp);
}

static ssize_t
store_pwm_enable(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index;
	unsigned long val;
	int i;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;
	if (val < 1 || val > 2)
		return -EINVAL;

#ifndef CONFIG_SENSORS_W83795_FANCTRL
	if (val > 1) {
		dev_warn(dev, "Automatic fan speed control support disabled\n");
		dev_warn(dev, "Build with CONFIG_SENSORS_W83795_FANCTRL=y if you want it\n");
		return -EOPNOTSUPP;
	}
#endif

	mutex_lock(&data->update_lock);
	switch (val) {
	case 1:
		/*                              */
		data->pwm_fcms[0] &= ~(1 << index);
		w83795_write(client, W83795_REG_FCMS1, data->pwm_fcms[0]);
		/*                                */
		for (i = 0; i < 6; i++) {
			data->pwm_tfmr[i] &= ~(1 << index);
			w83795_write(client, W83795_REG_TFMR(i),
				data->pwm_tfmr[i]);
		}
		break;
	case 2:
		data->pwm_fcms[0] |= (1 << index);
		w83795_write(client, W83795_REG_FCMS1, data->pwm_fcms[0]);
		break;
	}
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_pwm_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = w83795_update_pwm_config(dev);
	int index = to_sensor_dev_attr_2(attr)->index;
	unsigned int mode;

	if (data->pwm_fomc & (1 << index))
		mode = 0;	/*    */
	else
		mode = 1;	/*     */

	return sprintf(buf, "%u\n", mode);
}

/*
                                                               
                                                                  
           
 */
static int w83795_tss_useful(const struct w83795_data *data, int tsrc)
{
	int useful = 0, i;

	for (i = 0; i < 4; i++) {
		if (tss_map[i][tsrc] == TSS_MAP_RESERVED)
			continue;
		if (tss_map[i][tsrc] < 6)	/*        */
			useful += (data->has_temp >> tss_map[i][tsrc]) & 1;
		else				/*         */
			useful += (data->has_dts >> (tss_map[i][tsrc] - 6)) & 1;
	}

	return useful;
}

static ssize_t
show_temp_src(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	struct w83795_data *data = w83795_update_pwm_config(dev);
	int index = sensor_attr->index;
	u8 tmp = data->temp_src[index / 2];

	if (index & 1)
		tmp >>= 4;	/*                  */
	else
		tmp &= 0x0f;	/*                 */

	/*                                               */
	if (tmp >= 4 || tss_map[tmp][index] == TSS_MAP_RESERVED)
		return -EINVAL;		/*                  */

	return sprintf(buf, "%u\n", (unsigned int)tss_map[tmp][index] + 1);
}

static ssize_t
store_temp_src(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index;
	int tmp;
	unsigned long channel;
	u8 val = index / 2;

	if (kstrtoul(buf, 10, &channel) < 0 ||
	    channel < 1 || channel > 14)
		return -EINVAL;

	/*                                   */
	for (tmp = 0; tmp < 4; tmp++) {
		if (tss_map[tmp][index] == channel - 1)
			break;
	}
	if (tmp == 4)	/*          */
		return -EINVAL;

	mutex_lock(&data->update_lock);
	if (index & 1) {
		tmp <<= 4;
		data->temp_src[val] &= 0x0f;
	} else {
		data->temp_src[val] &= 0xf0;
	}
	data->temp_src[val] |= tmp;
	w83795_write(client, W83795_REG_TSS(val), data->temp_src[val]);
	mutex_unlock(&data->update_lock);

	return count;
}

#define TEMP_PWM_ENABLE   0
#define TEMP_PWM_FAN_MAP  1
static ssize_t
show_temp_pwm_enable(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	u8 tmp = 0xff;

	switch (nr) {
	case TEMP_PWM_ENABLE:
		tmp = (data->pwm_fcms[1] >> index) & 1;
		if (tmp)
			tmp = 4;
		else
			tmp = 3;
		break;
	case TEMP_PWM_FAN_MAP:
		tmp = data->pwm_tfmr[index];
		break;
	}

	return sprintf(buf, "%u\n", tmp);
}

static ssize_t
store_temp_pwm_enable(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned long tmp;

	if (kstrtoul(buf, 10, &tmp) < 0)
		return -EINVAL;

	switch (nr) {
	case TEMP_PWM_ENABLE:
		if (tmp != 3 && tmp != 4)
			return -EINVAL;
		tmp -= 3;
		mutex_lock(&data->update_lock);
		data->pwm_fcms[1] &= ~(1 << index);
		data->pwm_fcms[1] |= tmp << index;
		w83795_write(client, W83795_REG_FCMS2, data->pwm_fcms[1]);
		mutex_unlock(&data->update_lock);
		break;
	case TEMP_PWM_FAN_MAP:
		mutex_lock(&data->update_lock);
		tmp = SENSORS_LIMIT(tmp, 0, 0xff);
		w83795_write(client, W83795_REG_TFMR(index), tmp);
		data->pwm_tfmr[index] = tmp;
		mutex_unlock(&data->update_lock);
		break;
	}
	return count;
}

#define FANIN_TARGET   0
#define FANIN_TOL      1
static ssize_t
show_fanin(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	u16 tmp = 0;

	switch (nr) {
	case FANIN_TARGET:
		tmp = fan_from_reg(data->target_speed[index]);
		break;
	case FANIN_TOL:
		tmp = data->tol_speed;
		break;
	}

	return sprintf(buf, "%u\n", tmp);
}

static ssize_t
store_fanin(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	switch (nr) {
	case FANIN_TARGET:
		val = fan_to_reg(SENSORS_LIMIT(val, 0, 0xfff));
		w83795_write(client, W83795_REG_FTSH(index), val >> 4);
		w83795_write(client, W83795_REG_FTSL(index), (val << 4) & 0xf0);
		data->target_speed[index] = val;
		break;
	case FANIN_TOL:
		val = SENSORS_LIMIT(val, 0, 0x3f);
		w83795_write(client, W83795_REG_TFTS, val);
		data->tol_speed = val;
		break;
	}
	mutex_unlock(&data->update_lock);

	return count;
}


static ssize_t
show_temp_pwm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	long tmp = temp_from_reg(data->pwm_temp[index][nr]);

	return sprintf(buf, "%ld\n", tmp);
}

static ssize_t
store_temp_pwm(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned long val;
	u8 tmp;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;
	val /= 1000;

	mutex_lock(&data->update_lock);
	switch (nr) {
	case TEMP_PWM_TTTI:
		val = SENSORS_LIMIT(val, 0, 0x7f);
		w83795_write(client, W83795_REG_TTTI(index), val);
		break;
	case TEMP_PWM_CTFS:
		val = SENSORS_LIMIT(val, 0, 0x7f);
		w83795_write(client, W83795_REG_CTFS(index), val);
		break;
	case TEMP_PWM_HCT:
		val = SENSORS_LIMIT(val, 0, 0x0f);
		tmp = w83795_read(client, W83795_REG_HT(index));
		tmp &= 0x0f;
		tmp |= (val << 4) & 0xf0;
		w83795_write(client, W83795_REG_HT(index), tmp);
		break;
	case TEMP_PWM_HOT:
		val = SENSORS_LIMIT(val, 0, 0x0f);
		tmp = w83795_read(client, W83795_REG_HT(index));
		tmp &= 0xf0;
		tmp |= val & 0x0f;
		w83795_write(client, W83795_REG_HT(index), tmp);
		break;
	}
	data->pwm_temp[index][nr] = val;
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t
show_sf4_pwm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;

	return sprintf(buf, "%u\n", data->sf4_reg[index][SF4_PWM][nr]);
}

static ssize_t
store_sf4_pwm(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	w83795_write(client, W83795_REG_SF4_PWM(index, nr), val);
	data->sf4_reg[index][SF4_PWM][nr] = val;
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t
show_sf4_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = w83795_update_pwm_config(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;

	return sprintf(buf, "%u\n",
		(data->sf4_reg[index][SF4_TEMP][nr]) * 1000);
}

static ssize_t
store_sf4_temp(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;
	val /= 1000;

	mutex_lock(&data->update_lock);
	w83795_write(client, W83795_REG_SF4_TEMP(index, nr), val);
	data->sf4_reg[index][SF4_TEMP][nr] = val;
	mutex_unlock(&data->update_lock);

	return count;
}


static ssize_t
show_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	struct w83795_data *data = w83795_update_device(dev);
	long temp = temp_from_reg(data->temp[index][nr]);

	if (nr == TEMP_READ)
		temp += (data->temp_read_vrlsb[index] >> 6) * 250;
	return sprintf(buf, "%ld\n", temp);
}

static ssize_t
store_temp(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	long tmp;

	if (kstrtol(buf, 10, &tmp) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->temp[index][nr] = temp_to_reg(tmp, -128, 127);
	w83795_write(client, W83795_REG_TEMP[index][nr], data->temp[index][nr]);
	mutex_unlock(&data->update_lock);
	return count;
}


static ssize_t
show_dts_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = dev_get_drvdata(dev);
	int tmp;

	if (data->enable_dts & 2)
		tmp = 5;
	else
		tmp = 6;

	return sprintf(buf, "%d\n", tmp);
}

static ssize_t
show_dts(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index;
	struct w83795_data *data = w83795_update_device(dev);
	long temp = temp_from_reg(data->dts[index]);

	temp += (data->dts_read_vrlsb[index] >> 6) * 250;
	return sprintf(buf, "%ld\n", temp);
}

static ssize_t
show_dts_ext(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	struct w83795_data *data = dev_get_drvdata(dev);
	long temp = temp_from_reg(data->dts_ext[nr]);

	return sprintf(buf, "%ld\n", temp);
}

static ssize_t
store_dts_ext(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	long tmp;

	if (kstrtol(buf, 10, &tmp) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->dts_ext[nr] = temp_to_reg(tmp, -128, 127);
	w83795_write(client, W83795_REG_DTS_EXT(nr), data->dts_ext[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}


static ssize_t
show_temp_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct w83795_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index;
	int tmp;

	if (data->temp_mode & (1 << index))
		tmp = 3;	/*               */
	else
		tmp = 4;	/*            */

	return sprintf(buf, "%d\n", tmp);
}

/*                                                   */
static ssize_t
store_temp_mode(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int index = sensor_attr->index;
	int reg_shift;
	unsigned long val;
	u8 tmp;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;
	if ((val != 4) && (val != 3))
		return -EINVAL;

	mutex_lock(&data->update_lock);
	if (val == 3) {
		/*               */
		val = 0x01;
		data->temp_mode |= 1 << index;
	} else if (val == 4) {
		/*            */
		val = 0x03;
		data->temp_mode &= ~(1 << index);
	}

	reg_shift = 2 * index;
	tmp = w83795_read(client, W83795_REG_TEMP_CTRL2);
	tmp &= ~(0x03 << reg_shift);
	tmp |= val << reg_shift;
	w83795_write(client, W83795_REG_TEMP_CTRL2, tmp);

	mutex_unlock(&data->update_lock);
	return count;
}


/*                */
static ssize_t
show_in(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	struct w83795_data *data = w83795_update_device(dev);
	u16 val = data->in[index][nr];
	u8 lsb_idx;

	switch (nr) {
	case IN_READ:
		/*                                                        */
		if ((index >= 17) &&
		    !((data->has_gain >> (index - 17)) & 1))
			val *= 8;
		break;
	case IN_MAX:
	case IN_LOW:
		lsb_idx = IN_LSB_SHIFT_IDX[index][IN_LSB_IDX];
		val <<= 2;
		val |= (data->in_lsb[lsb_idx][nr] >>
			IN_LSB_SHIFT_IDX[index][IN_LSB_SHIFT]) & 0x03;
		if ((index >= 17) &&
		    !((data->has_gain >> (index - 17)) & 1))
			val *= 8;
		break;
	}
	val = in_from_reg(index, val);

	return sprintf(buf, "%d\n", val);
}

static ssize_t
store_in(struct device *dev, struct device_attribute *attr,
	 const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int index = sensor_attr->index;
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	unsigned long val;
	u8 tmp;
	u8 lsb_idx;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;
	val = in_to_reg(index, val);

	if ((index >= 17) &&
	    !((data->has_gain >> (index - 17)) & 1))
		val /= 8;
	val = SENSORS_LIMIT(val, 0, 0x3FF);
	mutex_lock(&data->update_lock);

	lsb_idx = IN_LSB_SHIFT_IDX[index][IN_LSB_IDX];
	tmp = w83795_read(client, IN_LSB_REG(lsb_idx, nr));
	tmp &= ~(0x03 << IN_LSB_SHIFT_IDX[index][IN_LSB_SHIFT]);
	tmp |= (val & 0x03) << IN_LSB_SHIFT_IDX[index][IN_LSB_SHIFT];
	w83795_write(client, IN_LSB_REG(lsb_idx, nr), tmp);
	data->in_lsb[lsb_idx][nr] = tmp;

	tmp = (val >> 2) & 0xff;
	w83795_write(client, W83795_REG_IN[index][nr], tmp);
	data->in[index][nr] = tmp;

	mutex_unlock(&data->update_lock);
	return count;
}


#ifdef CONFIG_SENSORS_W83795_FANCTRL
static ssize_t
show_sf_setup(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	struct w83795_data *data = w83795_update_pwm_config(dev);
	u16 val = data->setup_pwm[nr];

	switch (nr) {
	case SETUP_PWM_UPTIME:
	case SETUP_PWM_DOWNTIME:
		val = time_from_reg(val);
		break;
	}

	return sprintf(buf, "%d\n", val);
}

static ssize_t
store_sf_setup(struct device *dev, struct device_attribute *attr,
	 const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sensor_attr =
	    to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	struct i2c_client *client = to_i2c_client(dev);
	struct w83795_data *data = i2c_get_clientdata(client);
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	switch (nr) {
	case SETUP_PWM_DEFAULT:
		val = SENSORS_LIMIT(val, 0, 0xff);
		break;
	case SETUP_PWM_UPTIME:
	case SETUP_PWM_DOWNTIME:
		val = time_to_reg(val);
		if (val == 0)
			return -EINVAL;
		break;
	}

	mutex_lock(&data->update_lock);
	data->setup_pwm[nr] = val;
	w83795_write(client, W83795_REG_SETUP_PWM(nr), val);
	mutex_unlock(&data->update_lock);
	return count;
}
#endif


#define NOT_USED			-1

/*
                                                                               
                             
 */
#define SENSOR_ATTR_IN(index) {						\
	SENSOR_ATTR_2(in##index##_input, S_IRUGO, show_in, NULL,	\
		IN_READ, index), \
	SENSOR_ATTR_2(in##index##_max, S_IRUGO | S_IWUSR, show_in,	\
		store_in, IN_MAX, index),				\
	SENSOR_ATTR_2(in##index##_min, S_IRUGO | S_IWUSR, show_in,	\
		store_in, IN_LOW, index),				\
	SENSOR_ATTR_2(in##index##_alarm, S_IRUGO, show_alarm_beep,	\
		NULL, ALARM_STATUS, index + ((index > 14) ? 1 : 0)), \
	SENSOR_ATTR_2(in##index##_beep, S_IWUSR | S_IRUGO,		\
		show_alarm_beep, store_beep, BEEP_ENABLE,		\
		index + ((index > 14) ? 1 : 0)) }

/*
                                                               
                             
 */
#define SENSOR_ATTR_FAN(index) {					\
	SENSOR_ATTR_2(fan##index##_input, S_IRUGO, show_fan,		\
		NULL, FAN_INPUT, index - 1), \
	SENSOR_ATTR_2(fan##index##_min, S_IWUSR | S_IRUGO,		\
		show_fan, store_fan_min, FAN_MIN, index - 1),	\
	SENSOR_ATTR_2(fan##index##_alarm, S_IRUGO, show_alarm_beep,	\
		NULL, ALARM_STATUS, index + 31),			\
	SENSOR_ATTR_2(fan##index##_beep, S_IWUSR | S_IRUGO,		\
		show_alarm_beep, store_beep, BEEP_ENABLE, index + 31) }

#define SENSOR_ATTR_PWM(index) {					\
	SENSOR_ATTR_2(pwm##index, S_IWUSR | S_IRUGO, show_pwm,		\
		store_pwm, PWM_OUTPUT, index - 1),			\
	SENSOR_ATTR_2(pwm##index##_enable, S_IWUSR | S_IRUGO,		\
		show_pwm_enable, store_pwm_enable, NOT_USED, index - 1), \
	SENSOR_ATTR_2(pwm##index##_mode, S_IRUGO,			\
		show_pwm_mode, NULL, NOT_USED, index - 1),		\
	SENSOR_ATTR_2(pwm##index##_freq, S_IWUSR | S_IRUGO,		\
		show_pwm, store_pwm, PWM_FREQ, index - 1),		\
	SENSOR_ATTR_2(pwm##index##_nonstop, S_IWUSR | S_IRUGO,		\
		show_pwm, store_pwm, PWM_NONSTOP, index - 1),		\
	SENSOR_ATTR_2(pwm##index##_start, S_IWUSR | S_IRUGO,		\
		show_pwm, store_pwm, PWM_START, index - 1),		\
	SENSOR_ATTR_2(pwm##index##_stop_time, S_IWUSR | S_IRUGO,	\
		show_pwm, store_pwm, PWM_STOP_TIME, index - 1),	 \
	SENSOR_ATTR_2(fan##index##_target, S_IWUSR | S_IRUGO, \
		show_fanin, store_fanin, FANIN_TARGET, index - 1) }

/*
                                                               
                             
 */
#define SENSOR_ATTR_DTS(index) {					\
	SENSOR_ATTR_2(temp##index##_type, S_IRUGO ,		\
		show_dts_mode, NULL, NOT_USED, index - 7),	\
	SENSOR_ATTR_2(temp##index##_input, S_IRUGO, show_dts,		\
		NULL, NOT_USED, index - 7),				\
	SENSOR_ATTR_2(temp##index##_crit, S_IRUGO | S_IWUSR, show_dts_ext, \
		store_dts_ext, DTS_CRIT, NOT_USED),			\
	SENSOR_ATTR_2(temp##index##_crit_hyst, S_IRUGO | S_IWUSR,	\
		show_dts_ext, store_dts_ext, DTS_CRIT_HYST, NOT_USED),	\
	SENSOR_ATTR_2(temp##index##_max, S_IRUGO | S_IWUSR, show_dts_ext, \
		store_dts_ext, DTS_WARN, NOT_USED),			\
	SENSOR_ATTR_2(temp##index##_max_hyst, S_IRUGO | S_IWUSR,	\
		show_dts_ext, store_dts_ext, DTS_WARN_HYST, NOT_USED),	\
	SENSOR_ATTR_2(temp##index##_alarm, S_IRUGO,			\
		show_alarm_beep, NULL, ALARM_STATUS, index + 17),	\
	SENSOR_ATTR_2(temp##index##_beep, S_IWUSR | S_IRUGO,		\
		show_alarm_beep, store_beep, BEEP_ENABLE, index + 17) }

/*
                                                               
                             
 */
#define SENSOR_ATTR_TEMP(index) {					\
	SENSOR_ATTR_2(temp##index##_type, S_IRUGO | (index < 4 ? S_IWUSR : 0), \
		show_temp_mode, store_temp_mode, NOT_USED, index - 1),	\
	SENSOR_ATTR_2(temp##index##_input, S_IRUGO, show_temp,		\
		NULL, TEMP_READ, index - 1),				\
	SENSOR_ATTR_2(temp##index##_crit, S_IRUGO | S_IWUSR, show_temp,	\
		store_temp, TEMP_CRIT, index - 1),			\
	SENSOR_ATTR_2(temp##index##_crit_hyst, S_IRUGO | S_IWUSR,	\
		show_temp, store_temp, TEMP_CRIT_HYST, index - 1),	\
	SENSOR_ATTR_2(temp##index##_max, S_IRUGO | S_IWUSR, show_temp,	\
		store_temp, TEMP_WARN, index - 1),			\
	SENSOR_ATTR_2(temp##index##_max_hyst, S_IRUGO | S_IWUSR,	\
		show_temp, store_temp, TEMP_WARN_HYST, index - 1),	\
	SENSOR_ATTR_2(temp##index##_alarm, S_IRUGO,			\
		show_alarm_beep, NULL, ALARM_STATUS,			\
		index + (index > 4 ? 11 : 17)),				\
	SENSOR_ATTR_2(temp##index##_beep, S_IWUSR | S_IRUGO,		\
		show_alarm_beep, store_beep, BEEP_ENABLE,		\
		index + (index > 4 ? 11 : 17)),				\
	SENSOR_ATTR_2(temp##index##_pwm_enable, S_IWUSR | S_IRUGO,	\
		show_temp_pwm_enable, store_temp_pwm_enable,		\
		TEMP_PWM_ENABLE, index - 1),				\
	SENSOR_ATTR_2(temp##index##_auto_channels_pwm, S_IWUSR | S_IRUGO, \
		show_temp_pwm_enable, store_temp_pwm_enable,		\
		TEMP_PWM_FAN_MAP, index - 1),				\
	SENSOR_ATTR_2(thermal_cruise##index, S_IWUSR | S_IRUGO,		\
		show_temp_pwm, store_temp_pwm, TEMP_PWM_TTTI, index - 1), \
	SENSOR_ATTR_2(temp##index##_warn, S_IWUSR | S_IRUGO,		\
		show_temp_pwm, store_temp_pwm, TEMP_PWM_CTFS, index - 1), \
	SENSOR_ATTR_2(temp##index##_warn_hyst, S_IWUSR | S_IRUGO,	\
		show_temp_pwm, store_temp_pwm, TEMP_PWM_HCT, index - 1), \
	SENSOR_ATTR_2(temp##index##_operation_hyst, S_IWUSR | S_IRUGO,	\
		show_temp_pwm, store_temp_pwm, TEMP_PWM_HOT, index - 1), \
	SENSOR_ATTR_2(temp##index##_auto_point1_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 0, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point2_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 1, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point3_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 2, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point4_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 3, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point5_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 4, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point6_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 5, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point7_pwm, S_IRUGO | S_IWUSR, \
		show_sf4_pwm, store_sf4_pwm, 6, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point1_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 0, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point2_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 1, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point3_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 2, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point4_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 3, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point5_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 4, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point6_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 5, index - 1),		\
	SENSOR_ATTR_2(temp##index##_auto_point7_temp, S_IRUGO | S_IWUSR,\
		show_sf4_temp, store_sf4_temp, 6, index - 1) }


static struct sensor_device_attribute_2 w83795_in[][5] = {
	SENSOR_ATTR_IN(0),
	SENSOR_ATTR_IN(1),
	SENSOR_ATTR_IN(2),
	SENSOR_ATTR_IN(3),
	SENSOR_ATTR_IN(4),
	SENSOR_ATTR_IN(5),
	SENSOR_ATTR_IN(6),
	SENSOR_ATTR_IN(7),
	SENSOR_ATTR_IN(8),
	SENSOR_ATTR_IN(9),
	SENSOR_ATTR_IN(10),
	SENSOR_ATTR_IN(11),
	SENSOR_ATTR_IN(12),
	SENSOR_ATTR_IN(13),
	SENSOR_ATTR_IN(14),
	SENSOR_ATTR_IN(15),
	SENSOR_ATTR_IN(16),
	SENSOR_ATTR_IN(17),
	SENSOR_ATTR_IN(18),
	SENSOR_ATTR_IN(19),
	SENSOR_ATTR_IN(20),
};

static const struct sensor_device_attribute_2 w83795_fan[][4] = {
	SENSOR_ATTR_FAN(1),
	SENSOR_ATTR_FAN(2),
	SENSOR_ATTR_FAN(3),
	SENSOR_ATTR_FAN(4),
	SENSOR_ATTR_FAN(5),
	SENSOR_ATTR_FAN(6),
	SENSOR_ATTR_FAN(7),
	SENSOR_ATTR_FAN(8),
	SENSOR_ATTR_FAN(9),
	SENSOR_ATTR_FAN(10),
	SENSOR_ATTR_FAN(11),
	SENSOR_ATTR_FAN(12),
	SENSOR_ATTR_FAN(13),
	SENSOR_ATTR_FAN(14),
};

static const struct sensor_device_attribute_2 w83795_temp[][28] = {
	SENSOR_ATTR_TEMP(1),
	SENSOR_ATTR_TEMP(2),
	SENSOR_ATTR_TEMP(3),
	SENSOR_ATTR_TEMP(4),
	SENSOR_ATTR_TEMP(5),
	SENSOR_ATTR_TEMP(6),
};

static const struct sensor_device_attribute_2 w83795_dts[][8] = {
	SENSOR_ATTR_DTS(7),
	SENSOR_ATTR_DTS(8),
	SENSOR_ATTR_DTS(9),
	SENSOR_ATTR_DTS(10),
	SENSOR_ATTR_DTS(11),
	SENSOR_ATTR_DTS(12),
	SENSOR_ATTR_DTS(13),
	SENSOR_ATTR_DTS(14),
};

static const struct sensor_device_attribute_2 w83795_pwm[][8] = {
	SENSOR_ATTR_PWM(1),
	SENSOR_ATTR_PWM(2),
	SENSOR_ATTR_PWM(3),
	SENSOR_ATTR_PWM(4),
	SENSOR_ATTR_PWM(5),
	SENSOR_ATTR_PWM(6),
	SENSOR_ATTR_PWM(7),
	SENSOR_ATTR_PWM(8),
};

static const struct sensor_device_attribute_2 w83795_tss[6] = {
	SENSOR_ATTR_2(temp1_source_sel, S_IWUSR | S_IRUGO,
		      show_temp_src, store_temp_src, NOT_USED, 0),
	SENSOR_ATTR_2(temp2_source_sel, S_IWUSR | S_IRUGO,
		      show_temp_src, store_temp_src, NOT_USED, 1),
	SENSOR_ATTR_2(temp3_source_sel, S_IWUSR | S_IRUGO,
		      show_temp_src, store_temp_src, NOT_USED, 2),
	SENSOR_ATTR_2(temp4_source_sel, S_IWUSR | S_IRUGO,
		      show_temp_src, store_temp_src, NOT_USED, 3),
	SENSOR_ATTR_2(temp5_source_sel, S_IWUSR | S_IRUGO,
		      show_temp_src, store_temp_src, NOT_USED, 4),
	SENSOR_ATTR_2(temp6_source_sel, S_IWUSR | S_IRUGO,
		      show_temp_src, store_temp_src, NOT_USED, 5),
};

static const struct sensor_device_attribute_2 sda_single_files[] = {
	SENSOR_ATTR_2(intrusion0_alarm, S_IWUSR | S_IRUGO, show_alarm_beep,
		      store_chassis_clear, ALARM_STATUS, 46),
#ifdef CONFIG_SENSORS_W83795_FANCTRL
	SENSOR_ATTR_2(speed_cruise_tolerance, S_IWUSR | S_IRUGO, show_fanin,
		store_fanin, FANIN_TOL, NOT_USED),
	SENSOR_ATTR_2(pwm_default, S_IWUSR | S_IRUGO, show_sf_setup,
		      store_sf_setup, SETUP_PWM_DEFAULT, NOT_USED),
	SENSOR_ATTR_2(pwm_uptime, S_IWUSR | S_IRUGO, show_sf_setup,
		      store_sf_setup, SETUP_PWM_UPTIME, NOT_USED),
	SENSOR_ATTR_2(pwm_downtime, S_IWUSR | S_IRUGO, show_sf_setup,
		      store_sf_setup, SETUP_PWM_DOWNTIME, NOT_USED),
#endif
};

static const struct sensor_device_attribute_2 sda_beep_files[] = {
	SENSOR_ATTR_2(intrusion0_beep, S_IWUSR | S_IRUGO, show_alarm_beep,
		      store_beep, BEEP_ENABLE, 46),
	SENSOR_ATTR_2(beep_enable, S_IWUSR | S_IRUGO, show_alarm_beep,
		      store_beep, BEEP_ENABLE, 47),
};

/*
                   
 */

static void w83795_init_client(struct i2c_client *client)
{
	struct w83795_data *data = i2c_get_clientdata(client);
	static const u16 clkin[4] = {	/*        */
		14318, 24000, 33333, 48000
	};
	u8 config;

	if (reset)
		w83795_write(client, W83795_REG_CONFIG, 0x80);

	/*                            */
	config = w83795_read(client, W83795_REG_CONFIG);
	if (!(config & W83795_REG_CONFIG_START)) {
		dev_info(&client->dev, "Enabling monitoring operations\n");
		w83795_write(client, W83795_REG_CONFIG,
			     config | W83795_REG_CONFIG_START);
	}

	data->clkin = clkin[(config >> 3) & 0x3];
	dev_dbg(&client->dev, "clkin = %u kHz\n", data->clkin);
}

static int w83795_get_device_id(struct i2c_client *client)
{
	int device_id;

	device_id = i2c_smbus_read_byte_data(client, W83795_REG_DEVICEID);

	/*
                                                                       
                                            
  */
	if (device_id < 0 || (device_id & 0xf0) != 0x50) {
		int alt_id;

		alt_id = i2c_smbus_read_byte_data(client,
						  W83795_REG_DEVICEID_A);
		if (alt_id == 0x50)
			device_id = alt_id;
	}

	return device_id;
}

/*                                                        */
static int w83795_detect(struct i2c_client *client,
			 struct i2c_board_info *info)
{
	int bank, vendor_id, device_id, expected, i2c_addr, config;
	struct i2c_adapter *adapter = client->adapter;
	unsigned short address = client->addr;
	const char *chip_name;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;
	bank = i2c_smbus_read_byte_data(client, W83795_REG_BANKSEL);
	if (bank < 0 || (bank & 0x7c)) {
		dev_dbg(&adapter->dev,
			"w83795: Detection failed at addr 0x%02hx, check %s\n",
			address, "bank");
		return -ENODEV;
	}

	/*                         */
	vendor_id = i2c_smbus_read_byte_data(client, W83795_REG_VENDORID);
	expected = bank & 0x80 ? 0x5c : 0xa3;
	if (vendor_id != expected) {
		dev_dbg(&adapter->dev,
			"w83795: Detection failed at addr 0x%02hx, check %s\n",
			address, "vendor id");
		return -ENODEV;
	}

	/*                 */
	device_id = w83795_get_device_id(client) |
		    (i2c_smbus_read_byte_data(client, W83795_REG_CHIPID) << 8);
	if ((device_id >> 4) != 0x795) {
		dev_dbg(&adapter->dev,
			"w83795: Detection failed at addr 0x%02hx, check %s\n",
			address, "device id\n");
		return -ENODEV;
	}

	/*
                                                            
                
  */
	if ((bank & 0x07) == 0) {
		i2c_addr = i2c_smbus_read_byte_data(client,
						    W83795_REG_I2C_ADDR);
		if ((i2c_addr & 0x7f) != address) {
			dev_dbg(&adapter->dev,
				"w83795: Detection failed at addr 0x%02hx, "
				"check %s\n", address, "i2c addr");
			return -ENODEV;
		}
	}

	/*
                                       
                                                                       
                                                                    
                  
  */
	if ((bank & 0x07) != 0)
		i2c_smbus_write_byte_data(client, W83795_REG_BANKSEL,
					  bank & ~0x07);
	config = i2c_smbus_read_byte_data(client, W83795_REG_CONFIG);
	if (config & W83795_REG_CONFIG_CONFIG48)
		chip_name = "w83795adg";
	else
		chip_name = "w83795g";

	strlcpy(info->type, chip_name, I2C_NAME_SIZE);
	dev_info(&adapter->dev, "Found %s rev. %c at 0x%02hx\n", chip_name,
		 'A' + (device_id & 0xf), address);

	return 0;
}

#ifdef CONFIG_SENSORS_W83795_FANCTRL
#define NUM_PWM_ATTRIBUTES	ARRAY_SIZE(w83795_pwm[0])
#define NUM_TEMP_ATTRIBUTES	ARRAY_SIZE(w83795_temp[0])
#else
#define NUM_PWM_ATTRIBUTES	4
#define NUM_TEMP_ATTRIBUTES	8
#endif

static int w83795_handle_files(struct device *dev, int (*fn)(struct device *,
			       const struct device_attribute *))
{
	struct w83795_data *data = dev_get_drvdata(dev);
	int err, i, j;

	for (i = 0; i < ARRAY_SIZE(w83795_in); i++) {
		if (!(data->has_in & (1 << i)))
			continue;
		for (j = 0; j < ARRAY_SIZE(w83795_in[0]); j++) {
			if (j == 4 && !data->enable_beep)
				continue;
			err = fn(dev, &w83795_in[i][j].dev_attr);
			if (err)
				return err;
		}
	}

	for (i = 0; i < ARRAY_SIZE(w83795_fan); i++) {
		if (!(data->has_fan & (1 << i)))
			continue;
		for (j = 0; j < ARRAY_SIZE(w83795_fan[0]); j++) {
			if (j == 3 && !data->enable_beep)
				continue;
			err = fn(dev, &w83795_fan[i][j].dev_attr);
			if (err)
				return err;
		}
	}

	for (i = 0; i < ARRAY_SIZE(w83795_tss); i++) {
		j = w83795_tss_useful(data, i);
		if (!j)
			continue;
		err = fn(dev, &w83795_tss[i].dev_attr);
		if (err)
			return err;
	}

	for (i = 0; i < ARRAY_SIZE(sda_single_files); i++) {
		err = fn(dev, &sda_single_files[i].dev_attr);
		if (err)
			return err;
	}

	if (data->enable_beep) {
		for (i = 0; i < ARRAY_SIZE(sda_beep_files); i++) {
			err = fn(dev, &sda_beep_files[i].dev_attr);
			if (err)
				return err;
		}
	}

	for (i = 0; i < data->has_pwm; i++) {
		for (j = 0; j < NUM_PWM_ATTRIBUTES; j++) {
			err = fn(dev, &w83795_pwm[i][j].dev_attr);
			if (err)
				return err;
		}
	}

	for (i = 0; i < ARRAY_SIZE(w83795_temp); i++) {
		if (!(data->has_temp & (1 << i)))
			continue;
		for (j = 0; j < NUM_TEMP_ATTRIBUTES; j++) {
			if (j == 7 && !data->enable_beep)
				continue;
			err = fn(dev, &w83795_temp[i][j].dev_attr);
			if (err)
				return err;
		}
	}

	if (data->enable_dts) {
		for (i = 0; i < ARRAY_SIZE(w83795_dts); i++) {
			if (!(data->has_dts & (1 << i)))
				continue;
			for (j = 0; j < ARRAY_SIZE(w83795_dts[0]); j++) {
				if (j == 7 && !data->enable_beep)
					continue;
				err = fn(dev, &w83795_dts[i][j].dev_attr);
				if (err)
					return err;
			}
		}
	}

	return 0;
}

/*                                                    */
static int device_remove_file_wrapper(struct device *dev,
				      const struct device_attribute *attr)
{
	device_remove_file(dev, attr);
	return 0;
}

static void w83795_check_dynamic_in_limits(struct i2c_client *client)
{
	struct w83795_data *data = i2c_get_clientdata(client);
	u8 vid_ctl;
	int i, err_max, err_min;

	vid_ctl = w83795_read(client, W83795_REG_VID_CTRL);

	/*                                            */
	if ((vid_ctl & 0x07) == 0x00 || (vid_ctl & 0x07) == 0x07)
		return;

	data->has_dyn_in = (vid_ctl >> 3) & 0x07;
	for (i = 0; i < 2; i++) {
		if (!(data->has_dyn_in & (1 << i)))
			continue;

		/*                                                     */
		err_max = sysfs_chmod_file(&client->dev.kobj,
					   &w83795_in[i][2].dev_attr.attr,
					   S_IRUGO);
		err_min = sysfs_chmod_file(&client->dev.kobj,
					   &w83795_in[i][3].dev_attr.attr,
					   S_IRUGO);
		if (err_max || err_min)
			dev_warn(&client->dev, "Failed to set in%d limits "
				 "read-only (%d, %d)\n", i, err_max, err_min);
		else
			dev_info(&client->dev, "in%d limits set dynamically "
				 "from VID\n", i);
	}
}

/*                                                                          */
static void w83795_apply_temp_config(struct w83795_data *data, u8 config,
				     int temp_chan, int in_chan)
{
	/*                         */
	switch (config) {
	case 0x2: /*                    */
		data->has_in |= 1 << in_chan;
		break;
	case 0x1: /*               */
		if (temp_chan >= 4)
			break;
		data->temp_mode |= 1 << temp_chan;
		/*              */
	case 0x3: /*            */
		data->has_temp |= 1 << temp_chan;
		break;
	}
}

static int w83795_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int i;
	u8 tmp;
	struct device *dev = &client->dev;
	struct w83795_data *data;
	int err;

	data = kzalloc(sizeof(struct w83795_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	data->chip_type = id->driver_data;
	data->bank = i2c_smbus_read_byte_data(client, W83795_REG_BANKSEL);
	mutex_init(&data->update_lock);

	/*                     */
	w83795_init_client(client);

	/*                                           */
	data->has_in = w83795_read(client, W83795_REG_VOLT_CTRL1)
		     | (w83795_read(client, W83795_REG_VOLT_CTRL2) << 8);
	data->has_fan = w83795_read(client, W83795_REG_FANIN_CTRL1)
		      | (w83795_read(client, W83795_REG_FANIN_CTRL2) << 8);

	/*                                                                */
	tmp = w83795_read(client, W83795_REG_TEMP_CTRL1);
	if (tmp & 0x20)
		data->enable_dts = 1;
	w83795_apply_temp_config(data, (tmp >> 2) & 0x3, 5, 16);
	w83795_apply_temp_config(data, tmp & 0x3, 4, 15);
	tmp = w83795_read(client, W83795_REG_TEMP_CTRL2);
	w83795_apply_temp_config(data, tmp >> 6, 3, 20);
	w83795_apply_temp_config(data, (tmp >> 4) & 0x3, 2, 19);
	w83795_apply_temp_config(data, (tmp >> 2) & 0x3, 1, 18);
	w83795_apply_temp_config(data, tmp & 0x3, 0, 17);

	/*                         */
	if (data->enable_dts) {
		if (1 & w83795_read(client, W83795_REG_DTSC))
			data->enable_dts |= 2;
		data->has_dts = w83795_read(client, W83795_REG_DTSE);
	}

	/*                          */
	if (data->enable_dts == 1) {
		for (i = 0; i < 8; i++) {
			if (!(data->has_dts & (1 << i)))
				continue;
			tmp = w83795_read(client, W83795_REG_PECI_TBASE(i));
			dev_info(&client->dev,
				 "PECI agent %d Tbase temperature: %u\n",
				 i + 1, (unsigned int)tmp & 0x7f);
		}
	}

	data->has_gain = w83795_read(client, W83795_REG_VMIGB_CTRL) & 0x0f;

	/*                   */
	if (data->chip_type == w83795g)
		data->has_pwm = 8;
	else
		data->has_pwm = 2;

	/*                                */
	if (data->chip_type == w83795g) {
		/*                                      */
		data->enable_beep = 1;
	} else {
		/*
                                                             
                    
   */
		tmp = w83795_read(client, W83795_REG_OVT_CFG);
		if ((tmp & OVT_CFG_SEL) == 0)
			data->enable_beep = 1;
	}

	err = w83795_handle_files(dev, device_create_file);
	if (err)
		goto exit_remove;

	if (data->chip_type == w83795g)
		w83795_check_dynamic_in_limits(client);

	data->hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	return 0;

exit_remove:
	w83795_handle_files(dev, device_remove_file_wrapper);
	kfree(data);
exit:
	return err;
}

static int w83795_remove(struct i2c_client *client)
{
	struct w83795_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	w83795_handle_files(&client->dev, device_remove_file_wrapper);
	kfree(data);

	return 0;
}


static const struct i2c_device_id w83795_id[] = {
	{ "w83795g", w83795g },
	{ "w83795adg", w83795adg },
	{ }
};
MODULE_DEVICE_TABLE(i2c, w83795_id);

static struct i2c_driver w83795_driver = {
	.driver = {
		   .name = "w83795",
	},
	.probe		= w83795_probe,
	.remove		= w83795_remove,
	.id_table	= w83795_id,

	.class		= I2C_CLASS_HWMON,
	.detect		= w83795_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(w83795_driver);

MODULE_AUTHOR("Wei Song, Jean Delvare <khali@linux-fr.org>");
MODULE_DESCRIPTION("W83795G/ADG hardware monitoring driver");
MODULE_LICENSE("GPL");