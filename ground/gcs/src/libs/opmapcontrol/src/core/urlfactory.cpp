/**
 ******************************************************************************
 *
 * @file       urlfactory.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   OPMapWidget
 * @{
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "urlfactory.h"
#include <QRegExp>
#include <QDomDocument>
#include <QtXml>

namespace core {
const double UrlFactory::EarthRadiusKm = 6378.137; // WGS-84

UrlFactory::UrlFactory()
{
    /// <summary>
    /// timeout for map connections
    /// </summary>

    Proxy.setType(QNetworkProxy::NoProxy);

    /// <summary>
    /// Gets or sets the value of the User-agent HTTP header.
    /// </summary>
    UserAgent = "Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US; rv:1.9.1.7) Gecko/20091221 Firefox/3.5.7";

    Timeout   = 5 * 1000;
    CorrectGoogleVersions     = true;
    isCorrectedGoogleVersions = false;
    UseGeocoderCache = true;
    UsePlacemarkCache = true;
}
UrlFactory::~UrlFactory()
{}
QString UrlFactory::TileXYToQuadKey(const int &tileX, const int &tileY, const int &levelOfDetail) const
{
    QString quadKey;

    for (int i = levelOfDetail; i > 0; i--) {
        char digit = '0';
        int mask   = 1 << (i - 1);
        if ((tileX & mask) != 0) {
            digit++;
        }
        if ((tileY & mask) != 0) {
            digit++;
            digit++;
        }
        quadKey.append(digit);
    }
    return quadKey;
}
int UrlFactory::GetServerNum(const Point &pos, const int &max) const
{
#ifdef DEBUG_URLFACTORY
    qDebug() << QString("%1").arg(pos.X());
    qDebug() << QString("%1").arg(pos.Y());
#endif
    return (pos.X() + 2 * pos.Y()) % max;
}
void UrlFactory::setIsCorrectGoogleVersions(bool value)
{
    isCorrectedGoogleVersions = value;
}

bool UrlFactory::IsCorrectGoogleVersions()
{
    return isCorrectedGoogleVersions;
}

void UrlFactory::TryCorrectGoogleVersions()
{
    static bool versionRetrieved = false;

    if (versionRetrieved) {
        return;
    }
    QMutexLocker locker(&mutex);


    if (CorrectGoogleVersions && !IsCorrectGoogleVersions()) {
        QNetworkReply *reply;
        QNetworkRequest qheader;
        // This SSL Hack is half assed... technically bad *security* joojoo.
        // Required due to a QT5 bug on linux and Mac
        //
        QSslConfiguration conf = qheader.sslConfiguration();
        conf.setPeerVerifyMode(QSslSocket::VerifyNone);
        qheader.setSslConfiguration(conf);
        QNetworkAccessManager network;
        QEventLoop q;
        QTimer tT;
        tT.setSingleShot(true);
        connect(&network, SIGNAL(finished(QNetworkReply *)),
                &q, SLOT(quit()));
        connect(&tT, SIGNAL(timeout()), &q, SLOT(quit()));
        network.setProxy(Proxy);
#ifdef DEBUG_URLFACTORY
        qDebug() << "Correct GoogleVersion";
#endif // DEBUG_URLFACTORY
       // setIsCorrectGoogleVersions(true);
       // QString url = "https://www.google.com/maps/@0,-0,7z?dg=dbrw&newdg=1";
       // We need to switch to the Above url... the /lochp method will be depreciated soon
       // https://productforums.google.com/forum/#!category-topic/maps/navigation/k6EFrp7J7Jk
        QString url = "http://www.google.com/lochp";

        qheader.setUrl(QUrl(url));
        qheader.setRawHeader("User-Agent", UserAgent);
        reply = network.get(qheader);
        tT.start(Timeout);
        q.exec();
        if (!tT.isActive()) {
            return;
        }
        tT.stop();
        if ((reply->error() != QNetworkReply::NoError)) {
#ifdef DEBUG_URLFACTORY
            qDebug() << "Try corrected version withou abort or error:" << reply->errorString();
#endif // DEBUG_URLFACTORY
            return;
        }

        QString html = QString(reply->readAll());
#ifdef DEBUG_URLFACTORY
        qDebug() << html;
#endif // DEBUG_URLFACTORY

        QRegExp reg("\"*http://mts0.google.com/vt/lyrs=m@(\\d*)", Qt::CaseInsensitive);
        if (reg.indexIn(html) != -1) {
            QStringList gc = reg.capturedTexts();
            VersionGoogleMap = QString("m@%1").arg(gc[1]);
            VersionGoogleMapChina = VersionGoogleMap;
            VersionGoogleMapKorea = VersionGoogleMap;
#ifdef DEBUG_URLFACTORY
            qDebug() << "TryCorrectGoogleVersions, VersionGoogleMap: " << VersionGoogleMap;
#endif // DEBUG_URLFACTORY
        }

        reg = QRegExp("\"*http://mts0.google.com/vt/lyrs=h@(\\d*)", Qt::CaseInsensitive);
        if (reg.indexIn(html) != -1) {
            QStringList gc = reg.capturedTexts();
            VersionGoogleLabels = QString("h@%1").arg(gc[1]);
            VersionGoogleLabelsChina = VersionGoogleLabels;
            VersionGoogleLabelsKorea = VersionGoogleLabels;
#ifdef DEBUG_URLFACTORY
            qDebug() << "TryCorrectGoogleVersions, VersionGoogleLabels: " << VersionGoogleLabels;
#endif // DEBUG_URLFACTORY
        }

        reg = QRegExp("\"*http://khms0.google.com/kh/v=(\\d*)", Qt::CaseInsensitive);
        if (reg.indexIn(html) != -1) {
            QStringList gc = reg.capturedTexts();
            VersionGoogleSatellite = gc[1];
            VersionGoogleSatelliteKorea = VersionGoogleSatellite;
            VersionGoogleSatelliteChina = "s@" + VersionGoogleSatellite;

#ifdef DEBUG_URLFACTORY
            qDebug() << "TryCorrectGoogleVersions, VersionGoogleSatellite: " << VersionGoogleSatellite;
#endif // DEBUG_URLFACTORY
        }

        reg = QRegExp("\"*http://mts0.google.com/vt/lyrs=t@(\\d*),r@(\\d*)", Qt::CaseInsensitive);
        if (reg.indexIn(html) != -1) {
            QStringList gc = reg.capturedTexts();
            VersionGoogleTerrain = QString("t@%1,r@%2").arg(gc[1]).arg(gc[2]);
            VersionGoogleTerrainChina = VersionGoogleTerrain;
            VersionGoogleTerrainChina = VersionGoogleTerrain;
#ifdef DEBUG_URLFACTORY
            qDebug() << "TryCorrectGoogleVersions, VersionGoogleTerrain: " << VersionGoogleTerrain;
#endif // DEBUG_URLFACTORY
        }
        reply->deleteLater();
        versionRetrieved = true;
    }
}

QString UrlFactory::MakeImageUrl(const MapType::Types &type, const Point &pos, const int &zoom, const QString &language)
{
#ifdef DEBUG_URLFACTORY
    qDebug() << "Entered MakeImageUrl";
#endif // DEBUG_URLFACTORY
    switch (type) {
    case MapType::GoogleMap:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleMap).arg(language).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleSatellite:
    {
        QString server  = "khms";
        QString request = "kh";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleSatellite).arg(language).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleLabels:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleLabels).arg(language).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
#endif
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleLabels).arg(language).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleTerrain:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleTerrain).arg(language).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleMapChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        // http://mt0.google.cn/vt/v=w2.101&hl=zh-CN&gl=cn&x=12&y=6&z=4&s=Ga

        return QString("http://%1%2.google.cn/%3/lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleMapChina).arg("zh-CN").arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleSatelliteChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        // http://khm0.google.cn/kh/v=46&x=12&y=6&z=4&s=Ga

        return QString("http://%1%2.google.cn/%3/lyrs=%4&gl=cn&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleSatelliteChina).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleLabelsChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        // http://mt0.google.cn/vt/v=w2t.110&hl=zh-CN&gl=cn&x=12&y=6&z=4&s=Ga

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://%1%2.google.cn/%3/imgtp=png32&lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleLabelsChina).arg("zh-CN").arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
#endif
        return QString("http://%1%2.google.cn/%3/imgtp=png32&lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleLabelsChina).arg("zh-CN").arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleTerrainChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        // http://mt0.google.cn/vt/v=w2p.110&hl=zh-CN&gl=cn&x=12&y=6&z=4&s=Ga

        return QString("http://%1%2.google.cn/%3/lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleTerrainChina).arg("zh-CN").arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleMapKorea:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        // https://mts0.google.com/vt/lyrs=m@224000000&hl=ko&gl=KR&src=app&x=107&y=50&z=7&s=Gal
        // https://mts0.google.com/mt/v=kr1.11&hl=ko&x=109&y=49&z=7&s=

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://%1%2.google.com/%3/v=%4&hl=ko&gl=KR&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleMapKorea).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
#endif
        return QString("http://%1%2.google.com/%3/v=%4&hl=ko&gl=KR&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleMapKorea).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleSatelliteKorea:
    {
        QString server  = "khms";
        QString request = "kh";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();
        // http://khm1.google.co.kr/kh/v=54&x=109&y=49&z=7&s=

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://%1%2.google.co.kr/%3/v=%4&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleSatelliteKorea).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
#endif
        return QString("http://%1%2.google.co.kr/%3/v=%4&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleSatelliteKorea).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    case MapType::GoogleLabelsKorea:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        GetSecGoogleWords(pos, sec1, sec2);
        TryCorrectGoogleVersions();

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://%1%2.google.com/%3/v=%4&hl=ko&gl=KR&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleLabelsKorea).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
#endif
        return QString("http://%1%2.google.com/%3/v=%4&hl=ko&gl=KR&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(GetServerNum(pos, 4)).arg(request).arg(VersionGoogleLabelsKorea).arg(pos.X()).arg(sec1).arg(pos.Y()).arg(zoom).arg(sec2);
    }
    break;
    // *.yimg.com has been depreciated. "Here" is what Yahoo uses now. https://developer.here.com/rest-apis/documentation/enterprise-map-tile/topics/request-constructing.html
    case MapType::OpenStreetMap:
    {
        char letter = "abc"[GetServerNum(pos, 3)];
        return QString("http://%1.tile.openstreetmap.org/%2/%3/%4.png").arg(letter).arg(zoom).arg(pos.X()).arg(pos.Y());
    }
    break;
    // Need to update tile format to fit http://wiki.openstreetmap.org/wiki/Tile_servers
    case MapType::OpenStreetOsm:
    {
        char letter = "abc"[GetServerNum(pos, 3)];
#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://%1.tile.openstreetmap.org/%2/%3/%4.png").arg(letter).arg(zoom).arg(pos.X()).arg(pos.Y());
#endif
        return QString("http://%1.tile.openstreetmap.org/%2/%3/%4.png").arg(letter).arg(zoom).arg(pos.X()).arg(pos.Y());
    }
    break;
    case MapType::OpenStreetMapSurfer:
    {
        // http://wiki.openstreetmap.org/wiki/MapSurfer.NET -> Mapsurfernet.com -> dead
        // http://wiki.openstreetmap.org/wiki/OpenMapSurfer mentions: http://korona.geog.uni-heidelberg.de
        // http://korona.geog.uni-heidelberg.de/contact.html

        // OSM Roads layer: http://korona.geog.uni-heidelberg.de/tiles/roads/x={x}&y={y}&z={z}
        // or http://129.206.66.245:8001/tms_r.ashx?x={x}&y={y}&z={z}

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://korona.geog.uni-heidelberg.de/tiles/roads/x=%1&y=%2&z=%3").arg(pos.X()).arg(pos.Y()).arg(zoom);
#endif
        return QString("http://korona.geog.uni-heidelberg.de/tiles/roads/x=%1&y=%2&z=%3").arg(pos.X()).arg(pos.Y()).arg(zoom);
    }
    break;
    case MapType::OpenStreetMapSurferTerrain:
    {
        // ASTER GDEM & SRTM Hillshade layer: http://korona.geog.uni-heidelberg.de/tiles/asterh/
        // or http://129.206.66.245:8004/tms_hs.ashx?x={x}&y={y}&z={z}

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://korona.geog.uni-heidelberg.de/tiles/asterh/x=%1&y=%2&z=%3").arg(pos.X()).arg(pos.Y()).arg(zoom);
#endif
        return QString("http://korona.geog.uni-heidelberg.de/tiles/asterh/x=%1&y=%2&z=%3").arg(pos.X()).arg(pos.Y()).arg(zoom);
    }
    break;
    case MapType::BingMap:
    {
        QString key = TileXYToQuadKey(pos.X(), pos.Y(), zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=%3&mkt=%4%5").arg(GetServerNum(pos, 4)).arg(key).arg(VersionBingMaps).arg(language).arg(!(BingMapsClientToken.isNull() | BingMapsClientToken.isEmpty()) ? "&token=" + BingMapsClientToken : QString(""));
    }
    break;
    case MapType::BingSatellite:
    {
        QString key = TileXYToQuadKey(pos.X(), pos.Y(), zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/a%2.jpeg?g=%3&mkt=%4%5").arg(GetServerNum(pos, 4)).arg(key).arg(VersionBingMaps).arg(language).arg(!(BingMapsClientToken.isNull() | BingMapsClientToken.isEmpty()) ? "&token=" + BingMapsClientToken : QString(""));
    }
    break;
    case MapType::BingHybrid:
    {
        QString key = TileXYToQuadKey(pos.X(), pos.Y(), zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/h%2.jpeg?g=%3&mkt=%4%5").arg(GetServerNum(pos, 4)).arg(key).arg(VersionBingMaps).arg(language).arg(!(BingMapsClientToken.isNull() | BingMapsClientToken.isEmpty()) ? "&token=" + BingMapsClientToken : QString(""));
    }

    case MapType::ArcGIS_Map:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_StreetMap_World_2D/MapServer/tile/0/0/0.jpg

        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_StreetMap_World_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.Y()).arg(pos.X());
    }
    break;
    case MapType::ArcGIS_Satellite:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_Imagery_World_2D/MapServer/tile/1/0/1.jpg

        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_Imagery_World_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.Y()).arg(pos.X());
    }
    break;
    case MapType::ArcGIS_ShadedRelief:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_ShadedRelief_World_2D/MapServer/tile/1/0/1.jpg

#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://server.arcgisonline.com/ArcGIS/rest/services/World_Shaded_Relief/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.Y()).arg(pos.X());
#endif
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/World_Shaded_Relief/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.Y()).arg(pos.X());
    }
    break;
    case MapType::ArcGIS_Terrain:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/NGS_Topo_US_2D/MapServer/tile/4/3/15

        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/NGS_Topo_US_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.Y()).arg(pos.X());
    }
    break;
    case MapType::SigPacSpainMap:
    {
        // http://sigpac.magrama.es/fega/h5visor/ is new server location
#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://sigpac.magrama.es/SDG/raster/%1@3785/%2.%3.%4.img").arg(levelsForSigPacSpainMap[zoom]).arg(zoom).arg(pos.X()).arg((2 << (zoom - 1)) - pos.Y() - 1);
#endif
        return QString("http://sigpac.magrama.es/SDG/raster/%1@3785/%2.%3.%4.img").arg(levelsForSigPacSpainMap[zoom]).arg(zoom).arg(pos.X()).arg((2 << (zoom - 1)) - pos.Y() - 1);
    }
    break;
    case MapType::Statkart_Topo2:
    {
#ifdef DEBUG_URLFACTORY
        qDebug() << QString("http://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo2&zoom=%1&x=%2&y=%3").arg(zoom).arg(pos.X()).arg(pos.Y());
#endif
        return QString("http://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo2&zoom=%1&x=%2&y=%3").arg(zoom).arg(pos.X()).arg(pos.Y());
    }

    break;
    default:
        break;
    }

    return QString::null;
}
void UrlFactory::GetSecGoogleWords(const Point &pos, QString &sec1, QString &sec2)
{
    sec1 = ""; // after &x=...
    sec2 = ""; // after &zoom=...
    int seclen = ((pos.X() * 3) + pos.Y()) % 8;
    sec2 = SecGoogleWord.left(seclen);
    if (pos.Y() >= 10000 && pos.Y() < 100000) {
        sec1 = "&s=";
    }
}
QString UrlFactory::MakeGeocoderUrl(QString keywords)
{
    QString key = keywords.replace(' ', '+');

    // CSV output has been depreciated. API key is no longer needed.
    return QString("http://maps.googleapis.com/maps/api/geocode/xml?sensor=false&address=%1").arg(key);
}
QString UrlFactory::MakeReverseGeocoderUrl(internals::PointLatLng &pt, const QString &language)
{
#ifdef DEBUG_URLFACTORY
    qDebug() << "Language: " << language;
#else
    Q_UNUSED(language);
#endif
    // CSV output has been depreciated. API key is no longer needed.
    return QString("http://maps.googleapis.com/maps/api/geocode/xml?latlng=%1,%2").arg(QString::number(pt.Lat())).arg(QString::number(pt.Lng()));
}
internals::PointLatLng UrlFactory::GetLatLngFromGeodecoder(const QString &keywords, QString &status)
{
    return GetLatLngFromGeocoderUrl(MakeGeocoderUrl(keywords), UseGeocoderCache, status);
}

QString latxml;
QString lonxml;
// QString status;

internals::PointLatLng UrlFactory::GetLatLngFromGeocoderUrl(const QString &url, const bool &useCache, QString &status)
{
#ifdef DEBUG_URLFACTORY
    qDebug() << "Entered GetLatLngFromGeocoderUrl:";
#endif // DEBUG_URLFACTORY
    status = "ZERO_RESULTS";
    internals::PointLatLng ret(0, 0);
    QString urlEnd = url.mid(url.indexOf("geo?q=") + 6);
    urlEnd.replace(QRegExp(
                       "[^"
                       "A-Z,a-z,0-9,"
                       "\\^,\\&,\\',\\@,"
                       "\\{,\\},\\[,\\],"
                       "\\,,\\$,\\=,\\!,"
                       "\\-,\\#,\\(,\\),"
                       "\\%,\\.,\\+,\\~,\\_"
                       "]"), "_");

    QString geo = useCache ? Cache::Instance()->GetGeocoderFromCache(urlEnd) : "";

    if (geo.isNull() | geo.isEmpty()) {
#ifdef DEBUG_URLFACTORY
        qDebug() << "GetLatLngFromGeocoderUrl:Not in cache going internet";
#endif // DEBUG_URLFACTORY
        QNetworkReply *reply;
        QNetworkRequest qheader;
        // Lame hack *SSL security == none, bypass QT bug
        QSslConfiguration conf = qheader.sslConfiguration();
        conf.setPeerVerifyMode(QSslSocket::VerifyNone);
        qheader.setSslConfiguration(conf);
        QNetworkAccessManager network;
        network.setProxy(Proxy);
        qheader.setUrl(QUrl(url));
        qheader.setRawHeader("User-Agent", UserAgent);
        reply = network.get(qheader);
#ifdef DEBUG_URLFACTORY
        qDebug() << "GetLatLngFromGeocoderUrl:URL=" << url;
#endif // DEBUG_URLFACTORY
        QTime time;
        time.start();
        while ((!(reply->isFinished()) || (time.elapsed() > (6 * Timeout)))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        }
#ifdef DEBUG_URLFACTORY
        qDebug() << "Finished?" << reply->error() << " abort?" << (time.elapsed() > Timeout * 6);
#endif // DEBUG_URLFACTORY
        if ((reply->error() != QNetworkReply::NoError) | (time.elapsed() > Timeout * 6)) {
#ifdef DEBUG_URLFACTORY
            qDebug() << "GetLatLngFromGeocoderUrl::Network error";
#endif // DEBUG_URLFACTORY
            return internals::PointLatLng(0, 0);
        }
        {
            geo = reply->readAll();
#ifdef DEBUG_URLFACTORY
            qDebug() << "GetLatLngFromGeocoderUrl:Reply ok";
            qDebug() << geo; // This is the response from the geocode request (no longer in CSV)
#endif // DEBUG_URLFACTORY

            // This is SOOOO horribly hackish, code duplication needs to go. Needed a quick fix.
            QXmlStreamReader reader(geo);
            while (!reader.atEnd()) {
                reader.readNext();

                if (reader.isStartElement()) {
                    if (reader.name() == "lat") {
                        reader.readNext();
                        if (reader.atEnd()) {
                            break;
                        }

                        if (reader.isCharacters()) {
                            QString text = reader.text().toString();
#ifdef DEBUG_URLFACTORY
                            qDebug() << text;
#endif
                            latxml = text;
                            break;
                        }
                    }
                }
            }

            while (!reader.atEnd()) {
                reader.readNext();

                if (reader.isStartElement()) {
                    if (reader.name() == "lng") {
                        reader.readNext();
                        if (reader.atEnd()) {
                            break;
                        }

                        if (reader.isCharacters()) {
                            QString text = reader.text().toString();
#ifdef DEBUG_URLFACTORY
                            qDebug() << text;
#endif
                            lonxml = text;
                            break;
                        }
                    }
                }
            }

            QXmlStreamReader reader2(geo);
            while (!reader2.atEnd()) {
                reader2.readNext();

                if (reader2.isStartElement()) {
                    if (reader2.name() == "status") {
                        reader2.readNext();
                        if (reader2.atEnd()) {
                            break;
                        }

                        if (reader2.isCharacters()) {
                            QString text = reader2.text().toString();
#ifdef DEBUG_URLFACTORY
                            qDebug() << text;
#endif
                            status = text;
                            break;
                        }
                    }
                }
            }

            // cache geocoding
            if (useCache && geo.startsWith("200")) {
                Cache::Instance()->CacheGeocoder(urlEnd, geo);
            }
        }
        reply->deleteLater();
    }


    {
        if (status == "OK") {
            double lat = QString(latxml).toDouble();
            double lng = QString(lonxml).toDouble();

            ret = internals::PointLatLng(lat, lng);
#ifdef DEBUG_URLFACTORY
            qDebug() << "Status is: " << status;
            qDebug() << "Lat=" << lat << " Lng=" << lng;
#endif
        }
#ifdef DEBUG_URLFACTORY
        else if (status == "ZERO_RESULTS") {
            qDebug() << "No results";
        } else if (status == "OVER_QUERY_LIMIT") {
            qDebug() << "You are over quota on queries";
        } else if (status == "REQUEST_DENIED") {
            qDebug() << "Request was denied";
        } else if (status == "INVALID_REQUEST") {
            qDebug() << "Invalid request, missing address, lat long or location";
        } else if (status == "UNKNOWN_ERROR") {
            qDebug() << "Some sort of server error.";
        } else {
            qDebug() << "UrlFactory loop error";
        }
#endif
    }
    return ret;
}

Placemark UrlFactory::GetPlacemarkFromGeocoder(internals::PointLatLng location)
{
    return GetPlacemarkFromReverseGeocoderUrl(MakeReverseGeocoderUrl(location, LanguageStr), UsePlacemarkCache);
}

Placemark UrlFactory::GetPlacemarkFromReverseGeocoderUrl(const QString &url, const bool &useCache)
{
    Placemark ret("");

#ifdef DEBUG_URLFACTORY
    qDebug() << "Entered GetPlacemarkFromReverseGeocoderUrl:";
#endif // DEBUG_URLFACTORY
       // status = GeoCoderStatusCode::Unknow;
    QString urlEnd = url.right(url.indexOf("geo?hl="));
    urlEnd.replace(QRegExp(
                       "[^"
                       "A-Z,a-z,0-9,"
                       "\\^,\\&,\\',\\@,"
                       "\\{,\\},\\[,\\],"
                       "\\,,\\$,\\=,\\!,"
                       "\\-,\\#,\\(,\\),"
                       "\\%,\\.,\\+,\\~,\\_"
                       "]"), "_");

    QString reverse = useCache ? Cache::Instance()->GetPlacemarkFromCache(urlEnd) : "";

    if (reverse.isNull() | reverse.isEmpty()) {
#ifdef DEBUG_URLFACTORY
        qDebug() << "GetLatLngFromGeocoderUrl:Not in cache going internet";
#endif // DEBUG_URLFACTORY
        QNetworkReply *reply;
        QNetworkRequest qheader;
        QNetworkAccessManager network;
        network.setProxy(Proxy);
        qheader.setUrl(QUrl(url));
        qheader.setRawHeader("User-Agent", UserAgent);
        reply = network.get(qheader);
#ifdef DEBUG_URLFACTORY
        qDebug() << "GetLatLngFromGeocoderUrl:URL=" << url;
#endif // DEBUG_URLFACTORY
        QTime time;
        time.start();
        while ((!(reply->isFinished()) || (time.elapsed() > (6 * Timeout)))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        }
#ifdef DEBUG_URLFACTORY
        qDebug() << "Finished?" << reply->error() << " abort?" << (time.elapsed() > Timeout * 6);
#endif // DEBUG_URLFACTORY
        if ((reply->error() != QNetworkReply::NoError) | (time.elapsed() > Timeout * 6)) {
#ifdef DEBUG_URLFACTORY
            qDebug() << "GetLatLngFromGeocoderUrl::Network error";
#endif // DEBUG_URLFACTORY
            return ret;
        }
        {
#ifdef DEBUG_URLFACTORY
            qDebug() << "GetLatLngFromGeocoderUrl:Reply ok";
#endif // DEBUG_URLFACTORY
            QByteArray a = (reply->readAll());
            QTextCodec *codec = QTextCodec::codecForName("UTF-8");
            reverse = codec->toUnicode(a);
#ifdef DEBUG_URLFACTORY
            qDebug() << reverse;
#endif // DEBUG_URLFACTORY
            // cache geocoding
            if (useCache && reverse.startsWith("200")) {
                Cache::Instance()->CachePlacemark(urlEnd, reverse);
            }
        }
        reply->deleteLater();
    }


    // parse values
    // true : 200,4,56.1451640,22.0681787
    // false: 602,0,0,0
    if (reverse.startsWith("200")) {
        QString acc = reverse.left(reverse.indexOf('\"'));
        ret = Placemark(reverse.remove(reverse.indexOf('\"')));
        ret.SetAccuracy((int)(((QString)acc.split(',')[1]).toInt()));
    }
    return ret;
}
double UrlFactory::GetDistance(internals::PointLatLng p1, internals::PointLatLng p2)
{
    double dLat1InRad  = p1.Lat() * (M_PI / 180);
    double dLong1InRad = p1.Lng() * (M_PI / 180);
    double dLat2InRad  = p2.Lat() * (M_PI / 180);
    double dLong2InRad = p2.Lng() * (M_PI / 180);
    double dLongitude  = dLong2InRad - dLong1InRad;
    double dLatitude   = dLat2InRad - dLat1InRad;
    double a = pow(sin(dLatitude / 2), 2) + cos(dLat1InRad) * cos(dLat2InRad) * pow(sin(dLongitude / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double dDistance   = EarthRadiusKm * c;

    return dDistance;
}
}
