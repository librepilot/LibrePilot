/**
 ******************************************************************************
 *
 * @file       gpssnrwidget.cpp
 * @author     The LibrePilot Team, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GpsSnrWidget GpsSnr Widget Plugin
 * @{
 * @brief A widget for visualizing Signal to Noise Ratio information for known SV
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gpssnrwidget.h"
#include <QGraphicsRectItem>
#include <QFontMetrics>

GpsSnrWidget::GpsSnrWidget(QWidget *parent) :
    QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    // Create 'maxSatellites' signal to noise level bars:
    for (int i = 0; i < MAX_SATELLITES; i++) {
        satellites[i][0] = 0;
        satellites[i][1] = 0;
        satellites[i][2] = 0;
        satellites[i][3] = 0;

        // draw empty SNR bars and hide them for now
        boxes[i] = new QGraphicsRectItem();
        boxes[i]->setBrush(QColor("Green"));
        scene->addItem(boxes[i]);
        boxes[i]->hide();

        // draw PRN numbers
        QFont prnFont("Digital-7", 14); // font size 14 points
        satTexts[i] = new QGraphicsSimpleTextItem("###", boxes[i]);
        satTexts[i]->setBrush(QColor("Black"));
        satTexts[i]->setFont(prnFont);

        // Determine the dimensions (in pixels) of a piece of prn reference text.
        // This is used to make the scaling independent from the width of the actual prn text.
        QFontMetrics fm(prnFont);
        prnReferenceTextRect = fm.boundingRect("###");

        // draw SNR levels
        satSNRs[i] = new QGraphicsSimpleTextItem("##", boxes[i]);
        satSNRs[i]->setBrush(QColor("Black"));
        satSNRs[i]->setFont(QFont("Arial"));
    }
}

GpsSnrWidget::~GpsSnrWidget()
{
    delete scene;
    scene = 0;
}

void GpsSnrWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    scene->setSceneRect(0, 0, this->viewport()->width(), this->viewport()->height());
    for (int index = 0; index < MAX_SATELLITES; index++) {
        drawSat(index);
    }
}

void GpsSnrWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    scene->setSceneRect(0, 0, this->viewport()->width(), this->viewport()->height());
    for (int index = 0; index < MAX_SATELLITES; index++) {
        drawSat(index);
    }
}

void GpsSnrWidget::updateSat(int index, int prn, int elevation, int azimuth, int snr)
{
    if (index >= MAX_SATELLITES) {
        // A bit of error checking never hurts.
        return;
    }

    // TODO: add range checking
    satellites[index][0] = prn;
    satellites[index][1] = elevation;
    satellites[index][2] = azimuth;
    satellites[index][3] = snr;

    drawSat(index);
}

#define PRN_TEXTAREA_HEIGHT   20
#define SIDE_MARGIN           15
#define HIGH_SAT_AGING_CYCLES 10

void GpsSnrWidget::drawSat(int index)
{
    static int satsToShow = MAX_SATELLITES;
    static int lastNrVisibleSats = 0;
    static int highSatelliteCountAge = 0;
    bool heightLimited    = false;

    if (index >= MAX_SATELLITES) {
        // A bit of error checking never hurts.
        return;
    }

    /*
        Set the maximum number of satellites in the SNR widget based on the number
        of satellites previously seen.
        This code implements an aging timer to prevent flapping between different scales.

        There is a known issue: the current implementation can not differentiate between
        multiple sources of GPS information. For example a flight controller and a GPS mouse.

        In this case, the scale will always be set to the scale necessary for the
        source with the highest number of satellites in view.
     */
    if (index == 0) {
        if (lastNrVisibleSats > 16) {
            satsToShow = MAX_SATELLITES;
            highSatelliteCountAge = HIGH_SAT_AGING_CYCLES;
        } else if (highSatelliteCountAge > 0) {
            satsToShow = MAX_SATELLITES;
            --highSatelliteCountAge;
        } else {
            satsToShow = 16;
        }
        lastNrVisibleSats = 0;
    }

    const int prn = satellites[index][0];
    const int snr = satellites[index][3];
    if (prn && snr) {
        lastNrVisibleSats++;

        // When using integer values, width and height are the
        // box width and height, but the left and bottom borders are drawn on the box,
        // and the top and right borders are drawn just next to the box.
        // So the box seems one pixel wider and higher with a border.
        // I'm sure there's a good explanation for that :)

        // Casting to int rounds down, which is what I want.
        // Minus 2 to allow a pixel of white left and right.
        int availableWidth = (int)((scene->width() - 2 - 2 * SIDE_MARGIN) / satsToShow);

        // If there is no space, don't draw anything.
        if (availableWidth <= 0) {
            return;
        }

        boxes[index]->show();

        // 2 pixels extra one on each side.
        qreal width  = availableWidth - 2;
        // SNR = 1-99 (0 is special)..
        qreal height = int(((scene->height() - PRN_TEXTAREA_HEIGHT) / 99) * snr + 0.5);
        // 1 for showing a pixel of white extra to the left.
        qreal x = availableWidth * index + 1 + SIDE_MARGIN;
        // Rember, 0 is at the top.
        qreal y = scene->height() - height - PRN_TEXTAREA_HEIGHT;
        // Compensate for the extra pixel for the border.
        boxes[index]->setRect(0, 0, width - 1, height - 1);
        boxes[index]->setPos(x, y);

        QRectF boxRect = boxes[index]->boundingRect();

        // Show satellite constellations in a separate color
        // The UBX SVID numbers are defined in appendix A of u-blox8-M8_ReceiverDescrProtSpec_(UBX-13003221)_Public.pdf
        // GPS = default
        // SBAS 120-158, QZSS 193-197
        // BeiDou 33-64, 159-163
        // GLONASS 65-96, 255 if unidentified
        // Galileo 211-246
        if ((prn > 119 && prn < 159) || (prn > 192 && prn < 198)) {
            boxes[index]->setBrush(QColor("#fd700b")); // orange
        } else if ((prn > 64 && prn < 97) || 255 == prn) {
            boxes[index]->setBrush(QColor("Cyan"));
        } else if ((prn > 32 && prn < 65) || (prn > 158 && prn < 164)) {
            boxes[index]->setBrush(QColor("Red"));
        } else if (prn > 210 && prn < 247) {
            boxes[index]->setBrush(QColor("#e162f3")); // magenta
        } else {
            boxes[index]->setBrush(QColor("Green"));
        }

        // Add leading 0 to PRN numbers to make them at least two digits wide
        QString prnString = QString().number(prn);
        if (prnString.length() == 1) {
            prnString = "0" + prnString;
        }
        satTexts[index]->setText(prnString);
        QRectF textRect = satTexts[index]->boundingRect();

        // Reposition PRN number below the bar and rescale to fit the available space
        QTransform matrix;
        qreal scale = 0.85 * (boxRect.width() / prnReferenceTextRect.width());

        // Limit the maximum PRN text height to PRN_TEXTAREA_HEIGHT
        if ((prnReferenceTextRect.height() * scale) > (PRN_TEXTAREA_HEIGHT)) {
            scale = (PRN_TEXTAREA_HEIGHT) / prnReferenceTextRect.height();
            heightLimited = true;
        }

        matrix.translate(boxRect.width() / 2, boxRect.height());
        matrix.scale(scale, scale);
        matrix.translate(-textRect.width() / 2, 0);
        satTexts[index]->setTransform(matrix, false);


        // Add leading 0 to SNR values to make them at least two digits wide
        QString snrString = QString().number(snr);
        if (snrString.length() == 1) {
            snrString = "0" + snrString;
        }
        satSNRs[index]->setText(snrString);

        // Reposition SNR value above the bar and rescale to fit the available space
        matrix.reset();
        textRect = satSNRs[index]->boundingRect();
        if (!heightLimited) {
            scale = 0.85 * (boxRect.width() / prnReferenceTextRect.width());
        }
        matrix.translate(boxRect.width() / 2, 0);
        matrix.scale(scale, scale);
        matrix.translate(-textRect.width() / 2, -textRect.height());
        satSNRs[index]->setTransform(matrix, false);
    } else {
        boxes[index]->hide();
    }
}
