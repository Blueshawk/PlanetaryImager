/*
 * GuLinux Planetary Imager - https://github.com/GuLinux/PlanetaryImager
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "driverprotocol.h"
#include <algorithm>
#include <functional>
#include <opencv2/opencv.hpp>
#include <QJsonDocument>

using namespace std;
using namespace std::placeholders;
PROTOCOL_NAME_VALUE(Driver, CameraList);
PROTOCOL_NAME_VALUE(Driver, CameraListReply);
PROTOCOL_NAME_VALUE(Driver, CamerasParameter);
PROTOCOL_NAME_VALUE(Driver, ConnectCamera);
PROTOCOL_NAME_VALUE(Driver, CameraId);
PROTOCOL_NAME_VALUE(Driver, ConnectCameraReply);
PROTOCOL_NAME_VALUE(Driver, GetCameraName);
PROTOCOL_NAME_VALUE(Driver, GetCameraNameReply);
PROTOCOL_NAME_VALUE(Driver, CameraName);
PROTOCOL_NAME_VALUE(Driver, GetProperties);
PROTOCOL_NAME_VALUE(Driver, GetPropertiesReply);
PROTOCOL_NAME_VALUE(Driver, StartLive);
PROTOCOL_NAME_VALUE(Driver, StartLiveReply);
PROTOCOL_NAME_VALUE(Driver, ClearROI);
PROTOCOL_NAME_VALUE(Driver, GetControls);
PROTOCOL_NAME_VALUE(Driver, GetControlsReply);
PROTOCOL_NAME_VALUE(Driver, SendFrame);
PROTOCOL_NAME_VALUE(Driver, SetControl);
PROTOCOL_NAME_VALUE(Driver, signalFPS);
PROTOCOL_NAME_VALUE(Driver, FPS);
PROTOCOL_NAME_VALUE(Driver, signalTemperature);
PROTOCOL_NAME_VALUE(Driver, temp);
PROTOCOL_NAME_VALUE(Driver, signalControlChanged);
PROTOCOL_NAME_VALUE(Driver, signalDisconnected);

namespace {
  QVariant control2variant(const Imager::Control &control) {
    QVariantList choices;
    transform(begin(control.choices), end(control.choices), back_inserter(choices), [](const Imager::Control::Choice c){
      return QVariantMap{ {"label", c.label}, {"val", c.value} };
    });
    return QVariantMap {
      {"id",  control.id},
      {"name",  control.name},
      {"val",  control.value},
      {"def",  control.default_value},
      {"type",  static_cast<int>(control.type)},
      {"min",  control.range.min},
      {"max",  control.range.max},
      {"step",  control.range.step},
      {"choices", choices},
      {"decimals", control.decimals},
      {"is_duration", control.is_duration},
      {"has_auto", control.supports_auto},
      {"is_auto", control.value_auto},
      {"ro", control.readonly},
      {"duration_unit", control.duration_unit.count()},
    };
  }
  
  Imager::Control variant2control(const QVariant &variant) {
    QVariantMap ctrl = variant.toMap();
    Imager::Control::Choices choices;
    for(auto c: ctrl["choices"].toList()) {
      QVariantMap choice = c.toMap();
      choices.push_back({choice["label"].toString(), choice["val"]});
      qDebug() << "Choice: " << Imager::Control::Choice{choice["label"].toString(), choice["val"]};
    }
    return {
      ctrl["id"].toLongLong(),
      ctrl["name"].toString(),
      static_cast<Imager::Control::Type>(ctrl["type"].toInt()),
      ctrl["val"],
      ctrl["def"],
      { ctrl["min"], ctrl["max"], ctrl["step"] },
      choices,
      static_cast<qint16>(ctrl["decimals"].toInt()),
      ctrl["is_duration"].toBool(),
      ctrl["has_auto"].toBool(),
      ctrl["is_auto"].toBool(),
      ctrl["ro"].toBool(),
      std::chrono::duration<double>{ctrl["duration_unit"].toDouble()},
    };
  }
}

NetworkPacket::ptr DriverProtocol::sendCameraListReply(const Driver::Cameras& cameras)
{
  QVariantList v_cameras;
  transform(begin(cameras), end(cameras), back_inserter(v_cameras), [](const Driver::Camera::ptr &c){
    QVariantMap p;
    p["n"] = c->name();
    p["a"] = reinterpret_cast<qlonglong>(c.get());
    return p;
  });
  return packetCameraListReply() << NetworkPacket::Property{"cameras", v_cameras};
}



void DriverProtocol::decode(Driver::Cameras& cameras, const NetworkPacket::ptr& packet, const CameraFactory& factory)
{
  auto v_cameras = packet->property("cameras").toList();
  transform(begin(v_cameras), end(v_cameras), back_inserter(cameras), [&](const QVariant &v) { return factory(v.toMap()["n"].toString(), v.toMap()["a"].toLongLong()); });
}

NetworkPacket::ptr DriverProtocol::sendGetPropertiesReply(const Imager::Properties& properties)
{
  QVariantList l;
  QVariantList caps;
  transform(begin(properties.properties), end(properties.properties), back_inserter(l), [](const Imager::Properties::Property &p) {
    return QVariantMap {
      {"n", p.name},
      {"v", p.value},
      {"dn", p.display_name},
      {"dv", p.display_value},
    };
  });
  transform(begin(properties.capabilities), end(properties.capabilities), back_inserter(caps), [](const Imager::Capability &c) { return static_cast<int>(c); } );
  return packetGetPropertiesReply() << NetworkPacket::Property{"properties", l} << NetworkPacket::Property{"caps", caps};
}


void DriverProtocol::decode(Imager::Properties& properties, const NetworkPacket::ptr& packet)
{
  properties.capabilities.clear();
  properties.properties.clear();
  QVariantList p = packet->property("properties").toList();
  transform(begin(p), end(p), back_inserter(properties.properties), [](const QVariant &v){
    auto m = v.toMap();
    return Imager::Properties::Property{
      m["n"].toString(),
      m["v"].toString(),
      m["dn"].toString(),
      m["dv"].toString(),
    };
  });
  for(auto v: packet->property("caps").toList() )
    properties.capabilities.insert( static_cast<Imager::Capability>(v.toInt()) );
}

NetworkPacket::ptr DriverProtocol::sendGetControlsReply(const Imager::Controls& controls)
{
  QVariantList v;
  transform(begin(controls), end(controls), back_inserter(v), bind(control2variant, _1));
  //qDebug().noquote().nospace() << "controls encoded: " << QJsonDocument::fromVariant(v).toJson(QJsonDocument::Compact);
  return packetGetControlsReply() << NetworkPacket::Property{"controls", QJsonDocument::fromVariant(v).toBinaryData()};
}


void DriverProtocol::decode(Imager::Controls& controls, const NetworkPacket::ptr& packet)
{
  controls.clear();
  QVariantList variant_controls = QJsonDocument::fromBinaryData(packet->property("controls").toByteArray()).toVariant().toList();
  //qDebug().noquote().nospace() << "controls to decode: " << QJsonDocument::fromVariant(variant_controls).toJson(QJsonDocument::Compact);
  transform(begin(variant_controls), end(variant_controls), back_inserter(controls), bind(variant2control, _1));
}

NetworkPacket::ptr DriverProtocol::sendFrame(const Frame::ptr& frame)
{
  vector<uint8_t> data;
  QByteArray image;
  //cv::imencode(".jpg", frame->mat(), data);
  cv::imencode(frame->channels() == 1 ? ".pgm" : ".ppm", frame->mat(), data);
  image.resize(data.size());
  move(begin(data), end(data), begin(image));
  qDebug() << "FRAME data size: " << image.size() << ", bpp: " << frame->bpp() << ", res: " << frame->resolution() << ", channels: " << frame->channels();
  return packetSendFrame() << NetworkPacket::Property{"frame", image};
}

Frame::ptr DriverProtocol::decodeFrame(const NetworkPacket::ptr& packet)
{
  auto bytes = packet->property("frame").toByteArray();
  vector<uint8_t> data(bytes.size());
  move(begin(bytes), end(bytes), begin(data));
  auto mat = cv::imdecode(data, CV_LOAD_IMAGE_ANYDEPTH);
  auto frame = make_shared<Frame>(mat.channels() == 1 ? Frame::Mono : Frame::BGR, mat);
  qDebug() << "FRAME data size: " << bytes.size() << ", bpp: " << frame->bpp() << ", res: " << frame->resolution() << ", channels: " << frame->channels();
  
  return frame;
}

NetworkPacket::ptr DriverProtocol::control(const Imager::Control& control)
{
  return packetSetControl() << NetworkPacket::Property{"control", control2variant(control)};
}

Imager::Control DriverProtocol::decodeControl(const NetworkPacket::ptr &packet) {
  return variant2control(packet->property("control"));
}

