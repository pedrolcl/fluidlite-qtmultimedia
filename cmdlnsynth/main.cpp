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

#include <signal.h>
#include <QDebug>
#include <QScopedPointer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include "synthcontroller.h"
#include "programsettings.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    #define endl Qt::endl
#endif

static QScopedPointer<SynthController> synth;

void signalHandler(int sig)
{
    if (sig == SIGINT)
        qDebug() << "SIGINT received. Exiting" << endl;
    else if (sig == SIGTERM)
        qDebug() << "SIGTERM received. Exiting" << endl;
    if (!synth.isNull()) {
        synth->stop();
    }
    qApp->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("FluidLite");
    QCoreApplication::setApplicationName("fluidlite-cmdln");
    QCoreApplication::setApplicationVersion(QT_STRINGIFY(VERSION));
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    QCommandLineParser parser;
    parser.setApplicationDescription("Command Line MIDI Synthesizer");
    parser.addVersionOption();
    parser.addHelpOption();
    QCommandLineOption driverOption({"d", "driver"}, "MIDI Driver.", "driver");
    QCommandLineOption portOption({"p", "port"}, "MIDI Port.", "port");
    QCommandLineOption listOption({"s", "subs"}, "List available MIDI Ports.");
    QCommandLineOption bufferOption({"b", "buffer"},"Audio buffer time in milliseconds.", "buffer_time", "60");
    QCommandLineOption reverbOption({"r", "reverb"}, "Reverb type (none=0,presets=1,2,3,4,5).", "reverb_type", "3");
    QCommandLineOption wetOption({"w", "wet"}, "Reverb Level (0..100).", "reverb_level", "75");
    QCommandLineOption chorusOption({"c", "chorus"}, "Chorus type (none=0,active=1).", "chorus_type", "0");
    QCommandLineOption levelOption({"l", "level"}, "Chorus level (0..100).", "chorus_level", "0");
    QCommandLineOption deviceOption({"a", "audiodevice"}, "Audio Device Name", "device_name", "default");
    parser.addOption(driverOption);
    parser.addOption(portOption);
    parser.addOption(listOption);
    parser.addOption(bufferOption);
    parser.addOption(reverbOption);
    parser.addOption(chorusOption);
    parser.addOption(wetOption);
    parser.addOption(levelOption);
    parser.addOption(deviceOption);
    parser.addPositionalArgument("files", "SoundFont Files (.sf2;.sf3)", "[files ...]");
    parser.process(app);
    ProgramSettings::instance()->ReadFromNativeStorage();
    if (parser.isSet(driverOption)) {
        QString driverName = parser.value(driverOption);
        if (!driverName.isEmpty()) {
            ProgramSettings::instance()->setMidiDriver(driverName);
        }
    }
    if(parser.isSet(portOption)) {
        QString portName = parser.value(portOption);
        if (!portName.isEmpty()) {
            ProgramSettings::instance()->setPortName(portName);
        }
    }
    if (parser.isSet(bufferOption)) {
        int n = parser.value(bufferOption).toInt();
        if (n > 0)
            ProgramSettings::instance()->setBufferTime(n);
        else {
            fputs("Wrong buffer time.\n", stderr);
            parser.showHelp(1);
        }
    }
    if (parser.isSet(wetOption)) {
        int n = parser.value(wetOption).toInt();
        if (n >= 0 && n <= 100)
            ProgramSettings::instance()->setReverbLevel(n);
        else {
            fputs("Wrong reverb wet value.\n", stderr);
            parser.showHelp(1);
        }
    }
    if (parser.isSet(reverbOption)) {
        int n = parser.value(reverbOption).toInt();
        if (n >= 0 && n <= 5)
            ProgramSettings::instance()->setReverbType(n);
        else {
            fputs("Wrong reverb type.\n", stderr);
            parser.showHelp(1);
        }
    }
    if (parser.isSet(levelOption)) {
        int n = parser.value(levelOption).toInt();
        if (n >= 0 && n <= 100)
            ProgramSettings::instance()->setChorusLevel(n);
        else {
            fputs("Wrong chorus level.\n", stderr);
            parser.showHelp(1);
        }
    }
    if (parser.isSet(chorusOption)) {
        int n = parser.value(chorusOption).toInt();
        if (n >= 0 && n <= 1)
            ProgramSettings::instance()->setChorusType(n);
        else {
            fputs("Wrong chorus type.\n", stderr);
            parser.showHelp(1);
        }
    }
    if (parser.isSet(deviceOption)) {
        QString s = parser.value(deviceOption);
        if (!s.isEmpty()) {
            ProgramSettings::instance()->setAudioDeviceName(s);
        } else {
            fputs("Wrong Device Name.\n", stderr);
            parser.showHelp(1);
        }
    }
    synth.reset(new SynthController(ProgramSettings::instance()->bufferTime()));
    synth->renderer()->setMidiDriver(ProgramSettings::instance()->midiDriver());
    if (parser.isSet(listOption)) {
        auto avail = synth->renderer()->connections();
        fputs("Available MIDI Ports:\n", stdout);
        foreach(const auto &p, avail) {
            if (!p.isEmpty()) {
                fputs(p.toLocal8Bit(), stdout);
                fputs("\n", stdout);
            }
        }
        auto audioavail = synth->availableAudioDevices();
        fputs("Available Audio Devices:\n", stdout);
        foreach(const auto &p, audioavail) {
            if (!p.isEmpty()) {
                fputs(p.toLocal8Bit(), stdout);
                fputs("\n", stdout);
            }
        }
        return EXIT_SUCCESS;
    }
    QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        for(int i = 0; i < args.length();  ++i) {
            QFileInfo argFile(args[i]);
            if (argFile.exists()) {
                ProgramSettings::instance()->setSoundFontFile(argFile.filePath());
                synth->renderer()->openSoundfont(argFile.filePath());
            }
        }
    }
    synth->setAudioDeviceName(ProgramSettings::instance()->audioDeviceName());
    synth->renderer()->subscribe(ProgramSettings::instance()->portName());
    synth->renderer()->setReverbLevel(ProgramSettings::instance()->reverbLevel());
    synth->renderer()->initReverb(ProgramSettings::instance()->reverbType());
    synth->renderer()->setChorusLevel(ProgramSettings::instance()->chorusLevel());
    synth->renderer()->initChorus(ProgramSettings::instance()->chorusType());
    QObject::connect(synth.get(), &SynthController::underrunDetected, &app, []{
        fputs("Underrun error detected. Please increase the audio buffer size.\n", stderr);
    });
    QObject::connect(synth.get(), &SynthController::stallDetected, &app, []{
        fputs("Audio stall error detected. Please increase the audio buffer size.\n", stderr);
        synth->stop();
        qApp->quit();
    });
    //QObject::connect(&app, &QCoreApplication::aboutToQuit, synth.get(), &SynthController::stop);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, ProgramSettings::instance(), &ProgramSettings::SaveToNativeStorage);
    synth->start();
    return app.exec();
}
