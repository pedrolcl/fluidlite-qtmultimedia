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
#include <QMessageBox>
#include <drumstick/pianokeybd.h>
#include "mainwindow.h"
#include "programsettings.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_synth.reset(new SynthController(ProgramSettings::instance()->bufferTime()));
    m_synth->renderer()->setMidiDriver(ProgramSettings::instance()->midiDriver());
    m_synth->renderer()->subscribe(ProgramSettings::instance()->portName());
    m_synth->setAudioDeviceName(ProgramSettings::instance()->audioDeviceName());

    connect(m_synth.get(), &SynthController::underrunDetected, this, &MainWindow::underrunMessage);
    connect(m_synth.get(), &SynthController::stallDetected, this, &MainWindow::stallMessage);

    m_ui->setupUi(this);
    m_ui->combo_Reverb->addItem(QStringLiteral("Preset 1"), 1);
    m_ui->combo_Reverb->addItem(QStringLiteral("Preset 2"), 2);
    m_ui->combo_Reverb->addItem(QStringLiteral("Preset 3"), 3);
    m_ui->combo_Reverb->addItem(QStringLiteral("Preset 4"), 4);
    m_ui->combo_Reverb->addItem(QStringLiteral("None"), 0);
    m_ui->combo_Reverb->setCurrentIndex(4);

    m_ui->combo_Chorus->addItem(QStringLiteral("Preset 1"), 1);
    m_ui->combo_Chorus->addItem(QStringLiteral("Preset 2"), 2);
    m_ui->combo_Chorus->addItem(QStringLiteral("Preset 3"), 3);
    m_ui->combo_Chorus->addItem(QStringLiteral("Preset 4"), 4);
    m_ui->combo_Chorus->addItem(QStringLiteral("None"), 0);
    m_ui->combo_Chorus->setCurrentIndex(4);

    connect(m_ui->slider_Volume, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    connect(m_ui->spin_Buffer, SIGNAL(valueChanged(int)), this, SLOT(bufferSizeChanged(int)));
    connect(m_ui->spin_Octave, SIGNAL(valueChanged(int)), this, SLOT(octaveChanged(int)));
    connect(m_ui->combo_Audio, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceChanged(int)));
    connect(m_ui->combo_MIDI, SIGNAL(currentIndexChanged(int)), this, SLOT(subscriptionChanged(int)));
    connect(m_ui->combo_Reverb, SIGNAL(currentIndexChanged(int)), SLOT(reverbTypeChanged(int)));
    connect(m_ui->combo_Chorus, SIGNAL(currentIndexChanged(int)), SLOT(chorusTypeChanged(int)));
    connect(m_ui->dial_Reverb, &QDial::valueChanged, this, &MainWindow::reverbChanged);
    connect(m_ui->dial_Chorus, &QDial::valueChanged, this, &MainWindow::chorusChanged);
    connect(m_ui->openButton, &QToolButton::clicked, this, &MainWindow::openFile);
    connect(m_ui->pianoKeybd, &drumstick::widgets::PianoKeybd::noteOn, this, &MainWindow::noteOn);
    connect(m_ui->pianoKeybd, &drumstick::widgets::PianoKeybd::noteOff, this, &MainWindow::noteOff);
    connect(m_synth->renderer(), SIGNAL(midiNoteOn(int,int)), this, SLOT(showNoteOn(int,int)));
    connect(m_synth->renderer(), SIGNAL(midiNoteOff(int,int)), this, SLOT(showNoteOff(int,int)));
    m_sf2File = QString();
    initialize();
}

MainWindow::~MainWindow()
{
    m_synth->stop();
    delete m_ui;
}

void
MainWindow::initialize()
{
    m_ui->combo_MIDI->addItems(m_synth->renderer()->connections());
    m_ui->combo_MIDI->setCurrentText(ProgramSettings::instance()->portName());
    m_ui->combo_Audio->addItems(m_synth->availableAudioDevices());
    m_ui->combo_Audio->setCurrentText(m_synth->audioDeviceName());
    m_ui->spin_Buffer->setValue(ProgramSettings::instance()->bufferTime());
    int reverb = m_ui->combo_Reverb->findData(ProgramSettings::instance()->reverbType());
    m_ui->combo_Reverb->setCurrentIndex(reverb);
    m_ui->dial_Reverb->setValue(ProgramSettings::instance()->reverbWet()); //0..32765
    int chorus = m_ui->combo_Chorus->findData(ProgramSettings::instance()->chorusType());
    m_ui->combo_Chorus->setCurrentIndex(chorus);
    m_ui->dial_Chorus->setValue(ProgramSettings::instance()->chorusLevel());
    m_ui->pianoKeybd->setNumKeys(25, 0);
    m_ui->pianoKeybd->setShowLabels(drumstick::widgets::LabelVisibility::ShowMinimum);
    QFont f = QApplication::font();
    f.setPointSize(72);
    m_ui->pianoKeybd->setFont(f);
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
    int value = m_ui->combo_Reverb->itemData(index).toInt();
    m_synth->renderer()->initReverb(value);
    ProgramSettings::instance()->setReverbType(value);
    if (value < 0) {
        m_ui->dial_Reverb->setValue(0);
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
    int value = m_ui->combo_Chorus->itemData(index).toInt();
    m_synth->renderer()->initChorus(value);
    ProgramSettings::instance()->setChorusType(value);
    if (value < 0) {
        m_ui->dial_Chorus->setValue(0);
        ProgramSettings::instance()->setChorusLevel(0);
    }
}

void
MainWindow::chorusChanged(int value)
{
    m_synth->renderer()->setChorusLevel(value);
    ProgramSettings::instance()->setChorusLevel(value);
}

void MainWindow::deviceChanged(int value)
{
    //qDebug() << Q_FUNC_INFO << value;
    m_synth->setAudioDeviceName(m_ui->combo_Audio->itemText(value));
    ProgramSettings::instance()->setAudioDeviceName(m_ui->combo_Audio->itemText(value));
}

void MainWindow::subscriptionChanged(int value)
{
    //qDebug() << Q_FUNC_INFO << value << m_ui->combo_MIDI->itemText(value);
    m_synth->renderer()->subscribe(m_ui->combo_MIDI->itemText(value));
    ProgramSettings::instance()->setPortName(m_ui->combo_MIDI->itemText(value));
}

void MainWindow::bufferSizeChanged(int value)
{
    //qDebug() << Q_FUNC_INFO << value;
    m_synth->setBufferSize(value);
    ProgramSettings::instance()->setBufferTime(value);
}

void MainWindow::octaveChanged(int value)
{
    m_ui->pianoKeybd->setBaseOctave(value);
}

void MainWindow::volumeChanged(int value)
{
    //qDebug() << Q_FUNC_INFO << value;
    m_synth->setVolume(value);
    ProgramSettings::instance()->setVolumeLevel(value);
}


void
MainWindow::readFile(const QString &file)
{
    if (!file.isEmpty() && file != m_sf2File) {
        QFileInfo f(file);
        if (f.exists()) {
            m_sf2File = f.absoluteFilePath();
            m_ui->lblSong->setText(f.fileName());
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
        m_ui->lblSong->setText("[empty]");
    } else {
        readFile(songFile);
    }
}

void MainWindow::underrunMessage()
{
    static bool showing = false;
    if (!showing) {
        showing = true;
        QMessageBox::warning( this, "Underrun Error",
                              "Audio buffer underrun errors have been detected."
                              " Please increase the buffer time to avoid this problem.");
        showing = false;
    }
}

void MainWindow::stallMessage()
{
    QMessageBox::critical( this, "Audio Output Stalled",
                           "Audio output is stalled right now. Sound cannot be produced."
                           " Please increase the buffer time to avoid this problem.");
    m_synth->stop();
}

void MainWindow::noteOn(int midiNote, int vel)
{
    m_synth->renderer()->noteOn(0, midiNote, vel);
}

void MainWindow::noteOff(int midiNote, int vel)
{
    m_synth->renderer()->noteOff(0, midiNote, vel);
}

void MainWindow::showNoteOn(int midiNote, int vel)
{
    m_ui->pianoKeybd->showNoteOn(midiNote, vel);
}

void MainWindow::showNoteOff(int midiNote, int vel)
{
    m_ui->pianoKeybd->showNoteOff(midiNote, vel);
}
