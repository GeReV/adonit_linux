#ifndef TESTER_H
#define TESTER_H

#include <QtCore/QObject>
#include <gatocentralmanager.h>
#include <gatoperipheral.h>
#include <gatoservice.h>
#include <gatocharacteristic.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/uuid.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "endian.h"
#include "uinput.h"

class Tester : public QObject
{
	Q_OBJECT

public:
	explicit Tester(QObject *parent = 0);
	~Tester();

public slots:
	void test();

private slots:
	void handleDiscoveredPeripheral(GatoPeripheral *peripheral, int rssi);
	void handleDisconnected();
	void handleServices();
	void handleCharacteristics(const GatoService &service);
	void handleValueUpdated(const GatoCharacteristic &characteristic, const QByteArray &value);
	void handleReport(int p, int x, int y, int z);

	void discoverBluetoothDevice();
	void closeBluetoothDevice();
	void connectBluetoothDevice();
	void handleAdvertising(le_advertising_info *info, int rssi);

private:
	void parseName(bool complete, uint8_t data[], int len);
	void parseEIRUUIDs(int size, bool complete, uint8_t data[], int len);
	void parseEIR(uint8_t data[], int len);
	void connect(le_advertising_info *info);

	GatoCentralManager *manager;
	GatoPeripheral *peripheral;
	GatoCharacteristic agg_char;

	UInput *uinput;

	struct uinput_info info;
	struct uinput_user_dev dev;

	int dev_id;
	int hci = -1;
	int hci_fd = -1;
	int timeout = 1000;
	hci_filter hci_nf, hci_of;

	std::vector<bt_uuid_t> service_uuids;

	std::string name;
	bool complete_name;

    int fd;
    struct input_event event;

	int prev_btn_0;
	int prev_btn_1;
};

#endif // TESTER_H
