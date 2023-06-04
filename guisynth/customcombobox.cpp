/*
    FluidLite Synthesizer for Qt applications
    Copyright (C) 2022-2023, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "customcombobox.h"
#include <QPaintEvent>
#include <QRect>
#include <QStyleOptionComboBox>
#include <QStylePainter>

CustomComboBox::CustomComboBox(QWidget *parent)
    : QComboBox(parent)
{}

void CustomComboBox::paintEvent(QPaintEvent *event)
{
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QStylePainter p(this);
    p.drawComplexControl(QStyle::CC_ComboBox, opt);

    QRect textRect = style()->subControlRect(QStyle::CC_ComboBox,
                                             &opt,
                                             QStyle::SC_ComboBoxEditField,
                                             this);
    opt.currentText = p.fontMetrics().elidedText(opt.currentText, Qt::ElideRight, textRect.width());
    p.drawControl(QStyle::CE_ComboBoxLabel, opt);
}
