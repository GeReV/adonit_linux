#include <QtCore/QDebug>
#include <gatodescriptor.h>

#include "tester.h"

Tester::Tester(QObject *parent)
    : QObject(parent)
{
	manager = new GatoCentralManager(this);
	connect(manager, SIGNAL(discoveredPeripheral(GatoPeripheral*,int)),
	        SLOT(handleDiscoveredPeripheral(GatoPeripheral*,int)));

	uinput = new UInput();
}

Tester::~Tester()
{
	if (peripheral) {
		disconnect(peripheral, 0, this, 0);
		if (!agg_char.isNull()) {
			peripheral->setNotification(agg_char, false);
		}
		peripheral->disconnectPeripheral();
	}

	delete uinput;
}

void Tester::test()
{
	manager->scanForPeripherals();
}

void Tester::handleDiscoveredPeripheral(GatoPeripheral *peripheral, int rssi)
{
	qDebug() << "Found peripheral" << peripheral->address().toString() << peripheral->name();
//	if (peripheral->name() == "JN104FE9") {
		manager->stopScan();
		this->peripheral = peripheral;
		connect(peripheral, SIGNAL(connected()), SLOT(handleConnected()));
		connect(peripheral, SIGNAL(disconnected()), SLOT(handleDisconnected()));
		connect(peripheral, SIGNAL(servicesDiscovered()), SLOT(handleServices()));
		connect(peripheral, SIGNAL(characteristicsDiscovered(GatoService)), SLOT(handleCharacteristics(GatoService)));
		connect(peripheral, SIGNAL(valueUpdated(GatoCharacteristic,QByteArray)), SLOT(handleValueUpdated(GatoCharacteristic,QByteArray)));
		peripheral->connectPeripheral();
//	}
}

void Tester::handleConnected()
{
	qDebug() << "Peripheral connected";
	peripheral->discoverServices();

    info.create_mode = WDAEMON_CREATE;

	qDebug() << uinput->uinput_create(&info);
    qDebug() << uinput->wacom_set_events(&info);
    qDebug() << uinput->wacom_set_initial_values(&info, &dev);
	//	qDebug() << "Device created successfully!";
	//}
}

void Tester::handleDisconnected()
{
	qDebug() << "Peripheral disconnected";
}

void Tester::handleServices()
{
	qDebug() << "Services found";
	foreach (const GatoService &service, peripheral->services()) {
		if (service.uuid() == GatoUUID("dcd68980-aadc-11e1-a22a-0002a5d5c51b")) {
			// Found the service we want
			qDebug() << "Found service!";
			peripheral->discoverCharacteristics(service);
		}
	}
}

void Tester::handleCharacteristics(const GatoService &service)
{
	foreach (const GatoCharacteristic &c, service.characteristics()) {
		if (c.uuid() == GatoUUID("00002a26-0000-1000-8000-00805f9b34fb")) {
			peripheral->readValue(c);
		} else if (c.uuid() == GatoUUID("00002a5a-0000-1000-8000-00805f9b34fb")) {
			peripheral->setNotification(c, true);
			agg_char = c;
		}
	}
}

void Tester::handleValueUpdated(const GatoCharacteristic &characteristic, const QByteArray &value)
{
	if (characteristic.uuid() == agg_char.uuid()) {
		QDataStream s(value);
		s.setByteOrder(QDataStream::BigEndian);
		qint16 p, x, y, z;
		s >> p >> x >> y >> z;
		handleReport(p, x, y, z);
	} else {
		qDebug() << "Value updated" << characteristic.uuid() << QString(value);
	}
}

void Tester::handleReport(int p, int x, int y, int z)
{
	struct input_event ev;

	gettimeofday(&ev.time, 0);

	int btn_0 = (p & 0x1);
	int btn_1 = (p & 0x2) >> 1;

	if (btn_0 != prev_btn_0) {
		ev.type = EV_KEY;
		ev.code = BTN_0;
		ev.value = btn_0;

		uinput->uinput_write_event(&info, &ev);
	}
	prev_btn_0 = btn_0;

	if (btn_1 != prev_btn_1) {
		ev.type = EV_KEY;
		ev.code = BTN_1;
		ev.value = btn_1;

		uinput->uinput_write_event(&info, &ev);
	}
	prev_btn_1 = btn_1;

	ev.type = EV_ABS;
	ev.code = ABS_PRESSURE;
	ev.value = (p >> 5) & 0x7ff;
	uinput->uinput_write_event(&info, &ev);

	qDebug() << ev.time.tv_usec << ev.value << btn_0 << btn_1;
}
