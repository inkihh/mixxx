#include "preferences/dialog/dlgprefcolors.h"

#include <QLineEdit>
#include <QPainter>
#include <QTableView>

#include "dialog/dlgreplacecuecolor.h"
#include "library/basetracktablemodel.h"
#include "library/library.h"
#include "library/trackcollection.h"
#include "moc_dlgprefcolors.cpp"
#include "preferences/colorpaletteeditor.h"
#include "util/color/predefinedcolorpalettes.h"
#include "util/math.h"

namespace {

constexpr int kHotcueDefaultColorIndex = -1;
constexpr int kLoopDefaultColorIndex = -1;
constexpr QSize kPalettePreviewSize = QSize(108, 16);
const ConfigKey kAutoHotcueColorsConfigKey("[Controls]", "auto_hotcue_colors");
const ConfigKey kAutoLoopColorsConfigKey("[Controls]", "auto_loop_colors");
const ConfigKey kHotcueDefaultColorIndexConfigKey("[Controls]", "HotcueDefaultColorIndex");
const ConfigKey kLoopDefaultColorIndexConfigKey("[Controls]", "LoopDefaultColorIndex");
const ConfigKey kKeyColorsEnabledConfigKey("[Config]", "KeyColorsEnabled");

} // anonymous namespace

DlgPrefColors::DlgPrefColors(
        QWidget* parent,
        UserSettingsPointer pConfig,
        std::shared_ptr<Library> pLibrary)
        : DlgPreferencePage(parent),
          m_pConfig(pConfig),
          m_colorPaletteSettings(ColorPaletteSettings(pConfig)),
          m_pReplaceCueColorDlg(new DlgReplaceCueColor(
                  pConfig,
                  pLibrary->dbConnectionPool(),
                  pLibrary->trackCollectionManager(),
                  this)) {
    setupUi(this);
    comboBoxHotcueColors->setIconSize(kPalettePreviewSize);
    comboBoxTrackColors->setIconSize(kPalettePreviewSize);
    comboBoxKeyColors->setIconSize(kPalettePreviewSize);

    m_pReplaceCueColorDlg->setHidden(true);
    connect(m_pReplaceCueColorDlg,
            &DlgReplaceCueColor::databaseTracksChanged,
            &(pLibrary->trackCollectionManager()->internalCollection()->getTrackDAO()),
            &TrackDAO::slotDatabaseTracksChanged);

    connect(comboBoxHotcueColors,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefColors::slotHotcuePaletteIndexChanged);

    connect(pushButtonEditHotcuePalette,
            &QPushButton::clicked,
            this,
            &DlgPrefColors::slotEditHotcuePaletteClicked);

    connect(pushButtonEditTrackPalette,
            &QPushButton::clicked,
            this,
            &DlgPrefColors::slotEditTrackPaletteClicked);

    connect(pushButtonEditKeyPalette,
            &QPushButton::clicked,
            this,
            &DlgPrefColors::slotEditKeyPaletteClicked);

    connect(pushButtonReplaceCueColor,
            &QPushButton::clicked,
            this,
            &DlgPrefColors::slotReplaceCueColorClicked);

    connect(checkboxKeyColorsEnabled,
            &QCheckBox::stateChanged,
            this,
            &DlgPrefColors::slotKeyColorsEnabled);

    setScrollSafeGuardForAllInputWidgets(this);

    slotUpdate();
}

DlgPrefColors::~DlgPrefColors() {
}

void DlgPrefColors::slotUpdate() {
    comboBoxHotcueColors->clear();
    comboBoxTrackColors->clear();
    comboBoxKeyColors->clear();
    checkboxKeyColorsEnabled->setChecked(
            m_pConfig->getValue(kKeyColorsEnabledConfigKey,
                    BaseTrackTableModel::kKeyColorsEnabledDefault));
    for (const auto& palette : std::as_const(mixxx::PredefinedColorPalettes::kPalettes)) {
        QString paletteName = palette.getName();
        QIcon paletteIcon = drawPalettePreview(paletteName);
        comboBoxHotcueColors->addItem(paletteName);
        comboBoxHotcueColors->setItemIcon(
                comboBoxHotcueColors->count() - 1,
                paletteIcon);

        comboBoxTrackColors->addItem(paletteName);
        comboBoxTrackColors->setItemIcon(
                comboBoxTrackColors->count() - 1,
                paletteIcon);

        if (palette.size() == 12) {
            comboBoxKeyColors->addItem(paletteName);
            comboBoxKeyColors->setItemIcon(
                    comboBoxKeyColors->count() - 1,
                    paletteIcon);
        }
    }

    const QSet<QString> colorPaletteNames = m_colorPaletteSettings.getColorPaletteNames();
    for (const auto& paletteName : colorPaletteNames) {
        QIcon paletteIcon = drawPalettePreview(paletteName);
        comboBoxHotcueColors->addItem(paletteName);
        comboBoxHotcueColors->setItemIcon(
                comboBoxHotcueColors->count() - 1,
                paletteIcon);
        comboBoxTrackColors->addItem(paletteName);
        comboBoxTrackColors->setItemIcon(
                comboBoxHotcueColors->count() - 1,
                paletteIcon);
    }

    const ColorPalette trackPalette =
            m_colorPaletteSettings.getTrackColorPalette();
    comboBoxTrackColors->setCurrentText(
            trackPalette.getName());

    const ColorPalette hotcuePalette =
            m_colorPaletteSettings.getHotcueColorPalette();
    comboBoxHotcueColors->setCurrentText(
            hotcuePalette.getName());
    slotHotcuePaletteIndexChanged(comboBoxHotcueColors->currentIndex());

    const ColorPalette keyPalette =
            m_colorPaletteSettings.getKeyColorPalette();
    comboBoxKeyColors->setCurrentText(
            keyPalette.getName());

    bool autoHotcueColors = m_pConfig->getValue(kAutoHotcueColorsConfigKey, false);
    if (autoHotcueColors) {
        comboBoxHotcueDefaultColor->setCurrentIndex(0);
    } else {
        int hotcueDefaultColorIndex = m_pConfig->getValue(
                kHotcueDefaultColorIndexConfigKey, kHotcueDefaultColorIndex);
        if (hotcueDefaultColorIndex < 0 ||
                hotcueDefaultColorIndex >= hotcuePalette.size()) {
            hotcueDefaultColorIndex =
                    hotcuePalette.size() - 1; // default to last color (orange)
        }
        comboBoxHotcueDefaultColor->setCurrentIndex(
                hotcueDefaultColorIndex + 1);
    }

    bool autoLoopColors = m_pConfig->getValue(kAutoLoopColorsConfigKey, false);
    if (autoLoopColors) {
        comboBoxLoopDefaultColor->setCurrentIndex(0);
    } else {
        int loopDefaultColorIndex = m_pConfig->getValue(
                kLoopDefaultColorIndexConfigKey, kLoopDefaultColorIndex);
        if (loopDefaultColorIndex < 0 ||
                loopDefaultColorIndex >= hotcuePalette.size()) {
            loopDefaultColorIndex =
                    hotcuePalette.size() - 2; // default to second last color
            if (loopDefaultColorIndex < 0) {
                loopDefaultColorIndex = 0;
            }
        }
        comboBoxLoopDefaultColor->setCurrentIndex(
                loopDefaultColorIndex + 1);
    }
}

// Set the default values for all the widgets
void DlgPrefColors::slotResetToDefaults() {
    comboBoxHotcueColors->setCurrentText(
            mixxx::PredefinedColorPalettes::kDefaultHotcueColorPalette
                    .getName());
    comboBoxTrackColors->setCurrentText(
            mixxx::PredefinedColorPalettes::kDefaultTrackColorPalette
                    .getName());
    comboBoxKeyColors->setCurrentText(
            mixxx::PredefinedColorPalettes::kDefaultKeyColorPalette
                    .getName());
    comboBoxHotcueDefaultColor->setCurrentIndex(
            mixxx::PredefinedColorPalettes::kDefaultTrackColorPalette.size());
    comboBoxLoopDefaultColor->setCurrentIndex(
            mixxx::PredefinedColorPalettes::kDefaultTrackColorPalette.size() - 1);
    checkboxKeyColorsEnabled->setChecked(BaseTrackTableModel::kKeyColorsEnabledDefault);
}

// Apply and save any changes made in the dialog
void DlgPrefColors::slotApply() {
    QString hotcueColorPaletteName = comboBoxHotcueColors->currentText();
    QString trackColorPaletteName = comboBoxTrackColors->currentText();
    QString keyColorPaletteName = comboBoxKeyColors->currentText();
    bool bHotcueColorPaletteFound = false;
    bool bTrackColorPaletteFound = false;
    bool bKeyColorPaletteFound = false;

    for (const auto& palette :
            std::as_const(mixxx::PredefinedColorPalettes::kPalettes)) {
        if (!bHotcueColorPaletteFound &&
                hotcueColorPaletteName == palette.getName()) {
            m_colorPaletteSettings.setHotcueColorPalette(palette);
            bHotcueColorPaletteFound = true;
        }
        if (!bTrackColorPaletteFound &&
                trackColorPaletteName == palette.getName()) {
            m_colorPaletteSettings.setTrackColorPalette(palette);
            bTrackColorPaletteFound = true;
        }
        if (!bKeyColorPaletteFound &&
                keyColorPaletteName == palette.getName()) {
            m_colorPaletteSettings.setKeyColorPalette(palette);
            bKeyColorPaletteFound = true;
        }
    }

    if (!bHotcueColorPaletteFound) {
        m_colorPaletteSettings.setHotcueColorPalette(
                m_colorPaletteSettings.getColorPalette(hotcueColorPaletteName,
                        m_colorPaletteSettings.getHotcueColorPalette()));
    }

    if (!bTrackColorPaletteFound) {
        m_colorPaletteSettings.setTrackColorPalette(
                m_colorPaletteSettings.getColorPalette(trackColorPaletteName,
                        m_colorPaletteSettings.getTrackColorPalette()));
    }

    if (!bKeyColorPaletteFound) {
        m_colorPaletteSettings.setKeyColorPalette(
                m_colorPaletteSettings.getColorPalette(keyColorPaletteName,
                        m_colorPaletteSettings.getKeyColorPalette()));
    }

    int hotcueColorIndex = comboBoxHotcueDefaultColor->currentIndex();

    if (hotcueColorIndex > 0) {
        m_pConfig->setValue(kAutoHotcueColorsConfigKey, false);
        m_pConfig->setValue(kHotcueDefaultColorIndexConfigKey, hotcueColorIndex - 1);
    } else {
        m_pConfig->setValue(kAutoHotcueColorsConfigKey, true);
        m_pConfig->setValue(kHotcueDefaultColorIndexConfigKey, -1);
    }

    int loopColorIndex = comboBoxLoopDefaultColor->currentIndex();

    if (loopColorIndex > 0) {
        m_pConfig->setValue(kAutoLoopColorsConfigKey, false);
        m_pConfig->setValue(kLoopDefaultColorIndexConfigKey, loopColorIndex - 1);
    } else {
        m_pConfig->setValue(kAutoLoopColorsConfigKey, true);
        m_pConfig->setValue(kLoopDefaultColorIndexConfigKey, -1);
    }

    m_pConfig->setValue(kKeyColorsEnabledConfigKey, checkboxKeyColorsEnabled->checkState());
}

void DlgPrefColors::slotReplaceCueColorClicked() {
    ColorPaletteSettings colorPaletteSettings(m_pConfig);

    ColorPalette savedPalette = colorPaletteSettings.getHotcueColorPalette();
    ColorPalette newPalette = colorPaletteSettings.getColorPalette(
            comboBoxHotcueColors->currentText(), savedPalette);
    m_pReplaceCueColorDlg->setColorPalette(newPalette);

    int savedDefaultColorIndex = m_pConfig->getValue(
            kHotcueDefaultColorIndexConfigKey, savedPalette.size() - 1);
    if (savedDefaultColorIndex < 0) {
        savedDefaultColorIndex = 0;
    }
    mixxx::RgbColor savedDefaultColor =
            savedPalette.at(savedDefaultColorIndex % savedPalette.size());

    int newDefaultColorIndex = comboBoxHotcueDefaultColor->currentIndex() - 1;
    if (newDefaultColorIndex < 0) {
        newDefaultColorIndex = 0;
    }
    mixxx::RgbColor newDefaultColor = newPalette.at(newDefaultColorIndex % newPalette.size());

    m_pReplaceCueColorDlg->setCurrentColor(savedDefaultColor);
    if (savedDefaultColor != newDefaultColor) {
        m_pReplaceCueColorDlg->setNewColor(newDefaultColor);
    }

    m_pReplaceCueColorDlg->show();
    m_pReplaceCueColorDlg->raise();
    m_pReplaceCueColorDlg->activateWindow();
}

QPixmap DlgPrefColors::drawPalettePreview(const QString& paletteName) {
    ColorPalette palette = m_colorPaletteSettings.getHotcueColorPalette(paletteName);
    if (paletteName == palette.getName()) {
        QPixmap pixmap(kPalettePreviewSize);
        int count = math_max(palette.size(), 1);
        // Rounding up is required so the entire width of the pixmap is filled up to the edge.
        int widthPerColor = static_cast<int>(ceil(pixmap.width() / static_cast<float>(count)));
        QPainter painter(&pixmap);
        for (int i = 0; i < palette.size(); ++i) {
            painter.setPen(mixxx::RgbColor::toQColor(palette.at(i)));
            painter.setBrush(mixxx::RgbColor::toQColor(palette.at(i)));
            painter.drawRect(i * widthPerColor, 0, widthPerColor, pixmap.height());
        }
        return pixmap;
    }
    return QPixmap();
}

QIcon DlgPrefColors::drawHotcueColorByPaletteIcon(const QString& paletteName) {
    QPixmap pixmap(16, 16);
    QPainter painter(&pixmap);
    pixmap.fill(Qt::black);

    ColorPalette palette = m_colorPaletteSettings.getHotcueColorPalette(paletteName);
    if (paletteName == palette.getName()) {
        for (int i = 0; i < 4; ++i) {
            QColor color = mixxx::RgbColor::toQColor(
                    palette.colorForHotcueIndex(i));
            painter.setPen(color);
            painter.setBrush(color);
            painter.drawRect(0, i * 4, 16, 4);
        }
        return QIcon(pixmap);
    }
    return QIcon();
}

void DlgPrefColors::slotHotcuePaletteIndexChanged(int paletteIndex) {
    QString paletteName = comboBoxHotcueColors->itemText(paletteIndex);
    ColorPalette palette =
            m_colorPaletteSettings.getHotcueColorPalette(paletteName);

    int defaultHotcueColor = comboBoxHotcueDefaultColor->currentIndex();
    comboBoxHotcueDefaultColor->clear();

    int defaultLoopColor = comboBoxLoopDefaultColor->currentIndex();
    comboBoxLoopDefaultColor->clear();

    QIcon paletteIcon = drawHotcueColorByPaletteIcon(paletteName);

    comboBoxHotcueDefaultColor->addItem(tr("By hotcue number"), -1);
    comboBoxHotcueDefaultColor->setItemIcon(0, paletteIcon);

    comboBoxLoopDefaultColor->addItem(tr("By hotcue number"), -1);
    comboBoxLoopDefaultColor->setItemIcon(0, paletteIcon);

    QPixmap pixmap(16, 16);
    for (int i = 0; i < palette.size(); ++i) {
        QColor color = mixxx::RgbColor::toQColor(palette.at(i));
        pixmap.fill(color);
        QIcon icon(pixmap);
        QString item = tr("Color") + QStringLiteral(" ") +
                QString::number(i + 1) + QStringLiteral(": ") + color.name();

        comboBoxHotcueDefaultColor->addItem(item, i);
        comboBoxHotcueDefaultColor->setItemIcon(i + 1, icon);

        comboBoxLoopDefaultColor->addItem(item, i);
        comboBoxLoopDefaultColor->setItemIcon(i + 1, icon);
    }

    if (comboBoxHotcueDefaultColor->count() > defaultHotcueColor) {
        comboBoxHotcueDefaultColor->setCurrentIndex(defaultHotcueColor);
    } else {
        comboBoxHotcueDefaultColor->setCurrentIndex(
                comboBoxHotcueDefaultColor->count() - 1);
    }

    if (comboBoxLoopDefaultColor->count() > defaultLoopColor) {
        comboBoxLoopDefaultColor->setCurrentIndex(defaultLoopColor);
    } else {
        comboBoxLoopDefaultColor->setCurrentIndex(
                comboBoxLoopDefaultColor->count() - 1);
    }
}

void DlgPrefColors::slotEditTrackPaletteClicked() {
    QString trackColorPaletteName = comboBoxTrackColors->currentText();
    openColorPaletteEditor(trackColorPaletteName, &DlgPrefColors::trackPaletteUpdated);
}

void DlgPrefColors::slotEditHotcuePaletteClicked() {
    QString hotcueColorPaletteName = comboBoxHotcueColors->currentText();
    openColorPaletteEditor(hotcueColorPaletteName, &DlgPrefColors::hotcuePaletteUpdated);
}

void DlgPrefColors::slotEditKeyPaletteClicked() {
    QString keyColorPaletteName = comboBoxKeyColors->currentText();
    openColorPaletteEditor(keyColorPaletteName, &DlgPrefColors::keyPaletteUpdated);
}

void DlgPrefColors::slotKeyColorsEnabled(int i) {
    m_bKeyColorsEnabled = static_cast<bool>(i);
    BaseTrackTableModel::setKeyColorsEnabled(m_bKeyColorsEnabled);
    m_pConfig->setValue(kKeyColorsEnabledConfigKey, checkboxKeyColorsEnabled->checkState());
}

void DlgPrefColors::openColorPaletteEditor(
        const QString& paletteName,
        void (DlgPrefColors::*paletteUpdatedSlot)(const QString&)) {
    bool showHotcueNumbers = paletteUpdatedSlot == &DlgPrefColors::hotcuePaletteUpdated;
    std::unique_ptr<ColorPaletteEditor> pColorPaletteEditor =
            std::make_unique<ColorPaletteEditor>(this, showHotcueNumbers);

    connect(pColorPaletteEditor.get(),
            &ColorPaletteEditor::paletteChanged,
            this,
            paletteUpdatedSlot);

    connect(pColorPaletteEditor.get(),
            &ColorPaletteEditor::paletteRemoved,
            this,
            &DlgPrefColors::palettesUpdated);

    pColorPaletteEditor->initialize(m_pConfig, paletteName);
    pColorPaletteEditor->exec();
}

void DlgPrefColors::trackPaletteUpdated(const QString& trackColors) {
    QString hotcueColors = comboBoxHotcueColors->currentText();
    QString keyColors = comboBoxKeyColors->currentText();
    int defaultHotcueColor = comboBoxHotcueDefaultColor->currentIndex();
    int defaultLoopColor = comboBoxLoopDefaultColor->currentIndex();

    slotUpdate();
    restoreComboBoxes(hotcueColors, trackColors, keyColors, defaultHotcueColor, defaultLoopColor);
}

void DlgPrefColors::hotcuePaletteUpdated(const QString& hotcueColors) {
    QString trackColors = comboBoxTrackColors->currentText();
    QString keyColors = comboBoxKeyColors->currentText();
    int defaultHotcueColor = comboBoxHotcueDefaultColor->currentIndex();
    int defaultLoopColor = comboBoxLoopDefaultColor->currentIndex();

    slotUpdate();
    restoreComboBoxes(hotcueColors, trackColors, keyColors, defaultHotcueColor, defaultLoopColor);
}

void DlgPrefColors::keyPaletteUpdated(const QString& keyColors) {
    QString trackColors = comboBoxTrackColors->currentText();
    QString hotcueColors = comboBoxHotcueColors->currentText();
    int defaultHotcueColor = comboBoxHotcueDefaultColor->currentIndex();
    int defaultLoopColor = comboBoxLoopDefaultColor->currentIndex();

    slotUpdate();
    restoreComboBoxes(hotcueColors, trackColors, keyColors, defaultHotcueColor, defaultLoopColor);
}

void DlgPrefColors::palettesUpdated() {
    QString hotcueColors = comboBoxHotcueColors->currentText();
    QString trackColors = comboBoxTrackColors->currentText();
    QString keyColors = comboBoxKeyColors->currentText();
    int defaultHotcueColor = comboBoxHotcueDefaultColor->currentIndex();
    int defaultLoopColor = comboBoxLoopDefaultColor->currentIndex();

    slotUpdate();
    restoreComboBoxes(hotcueColors, trackColors, keyColors, defaultHotcueColor, defaultLoopColor);
}

void DlgPrefColors::restoreComboBoxes(
        const QString& hotcueColors,
        const QString& trackColors,
        const QString& keyColors,
        int defaultHotcueColor,
        int defaultLoopColor) {
    comboBoxHotcueColors->setCurrentText(hotcueColors);
    comboBoxTrackColors->setCurrentText(trackColors);
    comboBoxKeyColors->setCurrentText(keyColors);
    if (comboBoxHotcueDefaultColor->count() > defaultHotcueColor) {
        comboBoxHotcueDefaultColor->setCurrentIndex(defaultHotcueColor);
    } else {
        comboBoxHotcueDefaultColor->setCurrentIndex(
                comboBoxHotcueDefaultColor->count() - 1);
    }
    if (comboBoxLoopDefaultColor->count() > defaultLoopColor) {
        comboBoxLoopDefaultColor->setCurrentIndex(defaultLoopColor);
    } else {
        comboBoxLoopDefaultColor->setCurrentIndex(
                comboBoxLoopDefaultColor->count() - 1);
    }
}
