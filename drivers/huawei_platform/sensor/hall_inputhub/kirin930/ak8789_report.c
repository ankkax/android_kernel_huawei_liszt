#include "ak8789.h"

#ifdef HALL_DATA_REPORT_INPUTHUB
#include "../../../inputhub/kirin930/inputhub_route.h"
#include "../../../inputhub/kirin930/protocol.h"
extern int ap_hall_report(int value);
#endif

#include <linux/input.h>
#ifdef HALL_DATA_REPORT_INPUTDEV

#define HALL_INPUT_DEV	"hall"
#endif

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG ak8789
HWLOG_REGIST();

#ifdef HALL_DATA_REPORT_INPUTDEV
int ak8789_input_register(struct ak8789_data *data)
{
	int ret = 0;
	if (!data)
		return -ENOMEM;

	data->input_dev =  input_allocate_device();
	if (IS_ERR(data->input_dev)) {
		hwlog_err("input dev alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	data->input_dev->name = HALL_INPUT_DEV;
	set_bit(EV_MSC, data->input_dev->evbit);
	set_bit(MSC_SCAN, data->input_dev->mscbit);

	ret = input_register_device(data->input_dev);
	if (ret){
		hwlog_err("hw_input_hall regiset error %d\n", ret);
		input_free_device(data->input_dev);
	}
	return ret;
}

void ak8789_input_unregister(struct ak8789_data *data)
{
	if (data)
		input_unregister_device(data->input_dev);
}

int ak8789_report_data(void *data, packet_data pdata)
{
	struct input_dev *h_input = (struct input_dev *)data;
	input_event(h_input, EV_MSC, MSC_SCAN, pdata);
	input_sync(h_input);
	return 1;
}

EXPORT_SYMBOL_GPL(ak8789_report_data);
#endif

#ifdef HALL_DATA_REPORT_INPUTHUB
/*
 * Flip cover: the sensor HAL reports the hall state as a Huawei private sensor
 * that Android does not act on. Also expose it as a standard lid switch
 * (SW_LID, 1 = closed) so the framework turns the screen off and on with the
 * cover. Any magnet bit set means the cover is closed.
 */
static struct input_dev *ak8789_lid_dev;

int ak8789_lid_register(packet_data pdata)
{
	struct input_dev *dev = input_allocate_device();
	int ret;

	if (!dev)
		return -ENOMEM;
	dev->name = "hall_lid";
	dev->id.bustype = BUS_HOST;
	input_set_capability(dev, EV_SW, SW_LID);
	if (pdata)
		__set_bit(SW_LID, dev->sw);
	ret = input_register_device(dev);
	if (ret) {
		hwlog_err("lid input register error %d\n", ret);
		input_free_device(dev);
		return ret;
	}
	ak8789_lid_dev = dev;
	return 0;
}

int ak8789_report_data(void *data, packet_data pdata)
{
	if (ak8789_lid_dev) {
		input_report_switch(ak8789_lid_dev, SW_LID, pdata != 0);
		input_sync(ak8789_lid_dev);
	}
	return ap_hall_report(pdata);
}
EXPORT_SYMBOL_GPL(ak8789_report_data);
#endif
