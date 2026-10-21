// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "ocrtool.h"
#include "src/utils/confighandler.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

namespace {
// Appended for every OCR attempt so an offline machine can send the file
// back for diagnosis. Each step is written (and flushed, via close) BEFORE
// it runs, so a hard crash still leaves the failing step as the last line.
QString ocrLogPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/flameshot-ocr-ocr-log.txt";
}

void ocrLog(const QString& line)
{
    // Opt-in diagnostics: nothing is written unless the user enables
    // "Record OCR debug log" in the configuration.
    if (!ConfigHandler().ocrDebugLog()) {
        return;
    }
    QFile f(ocrLogPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ")
           << line << "\n";
    }
}

// The compiled-in default tessdata path of the vendor's libtesseract may
// differ from where the language packages install their files; these are
// the well-known locations on Debian-family systems (Kylin/UOS included).
QStringList tessdataCandidates()
{
    QStringList dirs;
    const QByteArray env = qgetenv("TESSDATA_PREFIX");
    if (!env.isEmpty()) {
        dirs << QString::fromLocal8Bit(env);
    }
    dirs << "/usr/share/tesseract-ocr/4.00/tessdata"
         << "/usr/share/tesseract-ocr/5/tessdata"
         << "/usr/share/tessdata";
    return dirs;
}
}

OcrTool::OcrTool(QObject* parent)
  : AbstractActionTool(parent)
{}

bool OcrTool::closeOnButtonPressed() const
{
    return true;
}

QIcon OcrTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "ocr.svg");
}

QString OcrTool::name() const
{
    return tr("OCR");
}

CaptureTool::Type OcrTool::type() const
{
    return CaptureTool::TYPE_OCR;
}

QString OcrTool::description() const
{
    return tr("Extract text from selection and copy to clipboard");
}

CaptureTool* OcrTool::copy(QObject* parent)
{
    return new OcrTool(parent);
}

void OcrTool::pressed(CaptureContext& context)
{
    emit requestAction(REQ_CLEAR_SELECTION);

    QPixmap pixmapToProcess = context.selectedScreenshotArea();

    qDebug() << "OCR: Processing image size:" << pixmapToProcess.width() << "x"
             << pixmapToProcess.height();

    QString extractedText = performOCR(pixmapToProcess);

    if (!extractedText.isEmpty()) {
        QApplication::clipboard()->setText(extractedText);
        qDebug() << "OCR: Text copied to clipboard";
    } else {
        qDebug() << "OCR: No text found";
    }

    emit requestAction(REQ_CAPTURE_DONE_OK);
    emit requestAction(REQ_CLOSE_GUI);
}

QString OcrTool::performOCR(const QPixmap& pixmap)
{
    QImage image = pixmap.toImage();

    if (image.width() < 300 || image.height() < 100) {
        image = image.scaled(image.width() * 2,
                             image.height() * 2,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }

    image = image.convertToFormat(QImage::Format_Grayscale8);
    image = image.convertToFormat(QImage::Format_RGB888);

    image.setDotsPerMeterX(2835);
    image.setDotsPerMeterY(2835);

    ocrLog(QString("=== OCR attempt: image %1x%2, tesseract %3, build %4")
             .arg(image.width())
             .arg(image.height())
             .arg(tesseract::TessBaseAPI::Version())
             .arg(FLAMESHOT_GIT_HASH));

    tesseract::TessBaseAPI api;
    int rc = 0;

    // Offline builds ship traineddata beside the app (under
    // APP_PREFIX/share/flameshot-ocr/tessdata); initializing from there
    // first makes OCR immune to the vendor libtesseract's compiled-in
    // default path and to whatever system language packages exist. When
    // the bundled data is absent (regular deb) the chain falls through to
    // the compiled-in default and then to well-known system locations.
    // End() the API after a failed Init before retrying.
    const QString bundledDir =
      QStringLiteral(APP_PREFIX) + "/share/flameshot-ocr/tessdata";
    struct InitAttempt
    {
        QString datapath;
        QString langs;
    };
    QList<InitAttempt> attempts;
    if (QFileInfo::exists(bundledDir + "/chi_sim.traineddata")) {
        attempts.append({ bundledDir, QStringLiteral("chi_sim+eng") });
    }
    attempts.append({ QString(), QStringLiteral("chi_sim+eng") });
    attempts.append({ QString(), QStringLiteral("eng") });
    for (const QString& dir : tessdataCandidates()) {
        attempts.append({ dir, QStringLiteral("eng") });
    }

    for (const InitAttempt& attempt : attempts) {
        const char* datapath =
          attempt.datapath.isNull()
            ? nullptr
            : attempt.datapath.toUtf8().constData();
        ocrLog(QString("step: Init(%1, %2)")
                 .arg(attempt.datapath.isNull() ? QStringLiteral("(default)")
                                                : attempt.datapath,
                      attempt.langs));
        rc = api.Init(datapath, attempt.langs.toUtf8().constData());
        ocrLog(QString("  rc=%1").arg(rc));
        if (rc == 0) {
            break;
        }
        api.End();
    }
    if (rc != 0) {
        ocrLog("RESULT: OCR FAILED - tesseract Init failed in every combination");
        ocrLog("traineddata presence per candidate directory:");
        const QStringList langs = { "chi_sim", "eng", "osd" };
        QStringList probeDirs = tessdataCandidates();
        probeDirs.prepend(bundledDir);
        for (const QString& dir : probeDirs) {
            for (const QString& lang : langs) {
                ocrLog(QString("  %1/%2.traineddata: %3")
                         .arg(dir, lang)
                         .arg(QFileInfo::exists(dir + "/" + lang + ".traineddata")
                                ? QStringLiteral("present")
                                : QStringLiteral("missing")));
            }
        }
        ocrLog(QString("log file: %1").arg(ocrLogPath()));
        return QString();
    }

    ocrLog(QString("step: init ok (datapath=%1, langs=%2)")
             .arg(api.GetDatapath())
             .arg(api.GetInitLanguagesAsString()));

    api.SetPageSegMode(tesseract::PSM_AUTO);

    api.SetVariable("debug_file", "/dev/null");

    ocrLog("step: SetImage");
    api.SetImage(
      image.bits(), image.width(), image.height(), 3, image.bytesPerLine());

    ocrLog("step: GetUTF8Text");
    char* outText = api.GetUTF8Text();
    QString result = QString::fromUtf8(outText);
    delete[] outText;
    api.End();

    ocrLog(QString("RESULT: OCR ok, %1 chars extracted")
             .arg(result.trimmed().size()));
    return result.trimmed();
}
