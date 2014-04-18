/****************************************************************************
** Meta object code from reading C++ file 'tester.h'
**
** Created: Fri Apr 18 02:44:27 2014
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tester.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tester.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Tester[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       8,    7,    7,    7, 0x0a,
      31,   15,    7,    7, 0x08,
      79,    7,    7,    7, 0x08,
      97,    7,    7,    7, 0x08,
     118,    7,    7,    7, 0x08,
     143,  135,    7,    7, 0x08,
     199,  178,    7,    7, 0x08,
     257,  249,    7,    7, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Tester[] = {
    "Tester\0\0test()\0peripheral,rssi\0"
    "handleDiscoveredPeripheral(GatoPeripheral*,int)\0"
    "handleConnected()\0handleDisconnected()\0"
    "handleServices()\0service\0"
    "handleCharacteristics(GatoService)\0"
    "characteristic,value\0"
    "handleValueUpdated(GatoCharacteristic,QByteArray)\0"
    "p,x,y,z\0handleReport(int,int,int,int)\0"
};

void Tester::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Tester *_t = static_cast<Tester *>(_o);
        switch (_id) {
        case 0: _t->test(); break;
        case 1: _t->handleDiscoveredPeripheral((*reinterpret_cast< GatoPeripheral*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->handleConnected(); break;
        case 3: _t->handleDisconnected(); break;
        case 4: _t->handleServices(); break;
        case 5: _t->handleCharacteristics((*reinterpret_cast< const GatoService(*)>(_a[1]))); break;
        case 6: _t->handleValueUpdated((*reinterpret_cast< const GatoCharacteristic(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 7: _t->handleReport((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Tester::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Tester::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Tester,
      qt_meta_data_Tester, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Tester::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Tester::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Tester::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Tester))
        return static_cast<void*>(const_cast< Tester*>(this));
    return QObject::qt_metacast(_clname);
}

int Tester::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
