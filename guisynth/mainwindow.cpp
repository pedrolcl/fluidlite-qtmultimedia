/*
    Sonivox EAS Synthesizer for Qt applications
    Copyright (C) 2016-2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

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

#include <QCloseEvent>
#include <QFileDialog>
#include "mainwindow.h"
#include "programsettings.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_synth = new SynthController(ProgramSettings::instance()->bufferTime(), this);
    m_synth->renderer()->setMidiDriver(ProgramSettings::instance()->midiDriver());
    m_synth->renderer()->subscribe(ProgramSettings::instance()->portName());
    m_synth->renderer()->setAudioDeviceName(ProgramSettings::instance()->audioDeviceName());

    ui->setupUi(this);
    ui->combo_Reverb->addItem(QStringLiteral("Preset 1"), 1);
    ui->combo_Reverb->addItem(QStringLiteral("Preset 2"), 2);
    ui->combo_Reverb->addItem(QStringLiteral("Preset 3"), 3);
    ui->combo_Reverb->addItem(QStringLiteral("Preset 4"), 4);
    ui->combo_Reverb->addItem(QStringLiteral("None"), 0);
    ui->combo_Reverb->setCurrentIndex(4);

    ui->combo_Chorus->addItem(QStringLiteral("Preset 1"), 1);
    ui->combo_Chorus->addItem(QStringLiteral("Preset 2"), 2);
    ui->combo_Chorus->addItem(QStringLiteral("Preset 3"), 3);
    ui->combo_Chorus->addItem(QStringLiteral("Preset 4"), 4);
    ui->combo_Chorus->addItem(QStringLiteral("None"), 0);
    ui->combo_Chorus->setCurrentIndex(4);

    connect(ui->combo_Reverb, SIGNAL(currentIndexChanged(int)), SLOT(reverbTypeChanged(int)));
    connect(ui->combo_Chorus, SIGNAL(currentIndexChanged(int)), SLOT(chorusTypeChanged(int)));
    connect(ui->dial_Reverb, &QDial::valueChanged, this, &MainWindow::reverbChanged);
    connect(ui->dial_Chorus, &QDial::valueChanged, this, &MainWindow::chorusChanged);
    connect(ui->openButton, &QToolButton::clicked, this, &MainWindow::openFile);
    m_sf2File = QString();
    initialize();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void
MainWindow::initialize()
{
    int bufTime = ProgramSettings::instance()->bufferTime();
    ui->bufTime->setText(QString("%1 ms").arg(bufTime));
    int reverb = ui->combo_Reverb->findData(ProgramSettings::instance()->reverbType());
    ui->combo_Reverb->setCurrentIndex(reverb);
    ui->dial_Reverb->setValue(ProgramSettings::instance()->reverbWet()); //0..32765
    int chorus = ui->combo_Chorus->findData(ProgramSettings::instance()->chorusType());
    ui->combo_Chorus->setCurrentIndex(chorus);
    ui->dial_Chorus->setValue(ProgramSettings::instance()->chorusLevel());
    ui->audioOutput->setText( m_synth->renderer()->audioDeviceName() );
    m_synth->start();
}

void
MainWindow::showEvent(QShowEvent* ev)
{
    ev->accept();
}

void
MainWindow::closeEvent(QCloseEvent* ev)
{
    m_synth->stop();
    ProgramSettings::instance()->SaveToNativeStorage();
    ev->accept();
}

void
MainWindow::reverbTypeChanged(int index)
{
    int value = ui->combo_Reverb->itemData(index).toInt();
    m_synth->renderer()->initReverb(value);
    ProgramSettings::instance()->setReverbType(value);
    if (value < 0) {
        ui->dial_Reverb->setValue(0);
        ProgramSettings::instance()->setReverbWet(0);
    }
}

void
MainWindow::reverbChanged(int value)
{
    m_synth->renderer()->setReverbWet(value);
    ProgramSettings::instance()->setReverbWet(value);
}

void
MainWindow::chorusTypeChanged(int index)
{
    int value = ui->combo_Chorus->itemData(index).toInt();
    m_synth->renderer()->initChorus(value);
    ProgramSettings::instance()->setChorusType(value);
    if (value < 0) {
        ui->dial_Chorus->setValue(0);
        ProgramSettings::instance()->setChorusLevel(0);
    }
}

void
MainWindow::chorusChanged(int value)
{
    m_synth->renderer()->setChorusLevel(value);
    ProgramSettings::instance()->setChorusLevel(value);
}

void
MainWindow::readFile(const QString &file)
{
    if (!file.isEmpty() && file != m_sf2File) {
        QFileInfo f(file);
        if (f.exists()) {
            m_sf2File = f.absoluteFilePath();
            ui->lblSong->setText(f.fileName());
            m_synth->renderer()->openSoundfont(m_sf2File);
            ProgramSettings::instance()->setSoundFontFile(m_sf2File);
        }
    }
}

void MainWindow::listPorts()
{
    auto avail = m_synth->renderer()->connections();
    fputs("Available MIDI Ports:\n", stdout);
    foreach(const auto &p, avail) {
        if (!p.isEmpty()) {
            fputs(p.toLocal8Bit(), stdout);
            fputs("\n", stdout);
        }
    }
}

void
MainWindow::openFile()
{
    QString songFile = QFileDialog::getOpenFileName(this,
        tr("Open SoundFont file"),  QDir::homePath(),
        tr("SoundFont Files (*.sf2 *.sf3)"));
    if (songFile.isEmpty()) {
        ui->lblSong->setText("[empty]");
    } else {
        readFile(songFile);
    }
}
