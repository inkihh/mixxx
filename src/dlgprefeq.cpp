/***************************************************************************
                          dlgprefeq.cpp  -  description
                             -------------------
    begin                : Thu Jun 7 2007
    copyright            : (C) 2007 by John Sully
    email                : jsully@scs.ryerson.ca
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QWidget>
#include <QString>
#include <QHBoxLayout>

#include "dlgprefeq.h"
#include "engine/enginefilteriir.h"
#include "controlobject.h"
#include "util/math.h"

#define CONFIG_KEY "[Mixer Profile]"
#define ENABLE_INTERNAL_EQ "EnableEQs"

const int kFrequencyUpperLimit = 20050;
const int kFrequencyLowerLimit = 16;

DlgPrefEQ::DlgPrefEQ(QWidget* pParent, EffectsManager* pEffectsManager,
                     ConfigObject<ConfigValue>* pConfig)
        : DlgPreferencePage(pParent),
          m_COTLoFreq(CONFIG_KEY, "LoEQFrequency"),
          m_COTHiFreq(CONFIG_KEY, "HiEQFrequency"),
          m_COTLoFi(CONFIG_KEY, "LoFiEQs"),
          m_COTEnableEq(CONFIG_KEY, ENABLE_INTERNAL_EQ),
          m_pConfig(pConfig),
          m_lowEqFreq(0.0),
          m_highEqFreq(0.0),
          m_pEffectsManager(pEffectsManager) {

    // Initialize the EQ Effect Rack
    int iEqRackNumber = m_pEffectsManager->getEffectChainManager()
                                                   ->getEffectRacksSize();
    m_pEQEffectRack = m_pEffectsManager->getEffectRack(iEqRackNumber - 1).data();

    setupUi(this);

    // Connection
    connect(SliderHiEQ, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateHiEQ()));
    connect(SliderHiEQ, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateHiEQ()));
    connect(SliderHiEQ, SIGNAL(sliderReleased()), this, SLOT(slotUpdateHiEQ()));

    connect(SliderLoEQ, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateLoEQ()));
    connect(SliderLoEQ, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateLoEQ()));
    connect(SliderLoEQ, SIGNAL(sliderReleased()), this, SLOT(slotUpdateLoEQ()));

    connect(CheckBoxLoFi, SIGNAL(stateChanged(int)), this, SLOT(slotLoFiChanged()));
    connect(CheckBoxEnbEQ, SIGNAL(stateChanged(int)), this, SLOT(slotEnaEQChanged()));

    connect(this, SIGNAL(effectOnChainSlot(const unsigned int,
                                           const unsigned int, QString)),
            m_pEQEffectRack, SLOT(slotLoadEffectOnChainSlot(const unsigned int,
                                           const unsigned int, QString)));

    // Add drop down lists for current decks and connect num_decks control
    // to slotAddComboBox
    m_pNumDecks = new ControlObjectSlave("[Master]", "num_decks", this);
    m_pNumDecks->connectValueChanged(SLOT(slotAddComboBox(double)));
    slotAddComboBox(m_pNumDecks->get());

    loadSettings();
    slotUpdate();
    slotApply();
}

DlgPrefEQ::~DlgPrefEQ() {
    qDeleteAll(m_deckEffectSelectors);
    m_deckEffectSelectors.clear();
}

void DlgPrefEQ::slotAddComboBox(double numDecks) {
    while (m_deckEffectSelectors.size() < static_cast<int>(numDecks)) {
        QHBoxLayout* innerHLayout = new QHBoxLayout();
        QLabel* label = new QLabel(QString("Deck %1").
                            arg(m_deckEffectSelectors.size() + 1), this);

        // Get the configured effect or eqdefault if none is configured
        QString configuredEffect = m_pConfig->getValueString(ConfigKey(CONFIG_KEY,
                QString("EffectForDeck%1").arg(m_deckEffectSelectors.size() + 1)),
                QString("org.mixxx.effects.eqdefault"));

        // Create the drop down list and populate it with the available effects
        QComboBox* box = new QComboBox(this);
        QStringList availableEffects(m_pEffectsManager->getAvailableEffects().toList());
        box->addItems(availableEffects);
        m_deckEffectSelectors.append(box);
        connect(box, SIGNAL(currentIndexChanged(QString)),
                this, SLOT(slotEffectChangedOnDeck(QString)));

        int selectedEffectIndex = box->findText(configuredEffect);
        box->setCurrentIndex(selectedEffectIndex);

        // Setup the GUI
        innerHLayout->addWidget(label);
        innerHLayout->addWidget(box);
        innerHLayout->addStretch();
        verticalLayout_2->addLayout(innerHLayout);
    }
}

void DlgPrefEQ::loadSettings() {
    QString highEqCourse = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "HiEQFrequency"));
    QString highEqPrecise = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "HiEQFrequencyPrecise"));
    QString lowEqCourse = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "LoEQFrequency"));
    QString lowEqPrecise = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "LoEQFrequencyPrecise"));

    double lowEqFreq = 0.0;
    double highEqFreq = 0.0;

    // Precise takes precedence over course.
    lowEqFreq = lowEqCourse.isEmpty() ? lowEqFreq : lowEqCourse.toDouble();
    lowEqFreq = lowEqPrecise.isEmpty() ? lowEqFreq : lowEqPrecise.toDouble();
    highEqFreq = highEqCourse.isEmpty() ? highEqFreq : highEqCourse.toDouble();
    highEqFreq = highEqPrecise.isEmpty() ? highEqFreq : highEqPrecise.toDouble();

    if (lowEqFreq == 0.0 || highEqFreq == 0.0 || lowEqFreq == highEqFreq) {
        CheckBoxLoFi->setChecked(true);
        setDefaultShelves();
        lowEqFreq = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "LoEQFrequencyPrecise")).toDouble();
        highEqFreq = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "HiEQFrequencyPrecise")).toDouble();
    }

    SliderHiEQ->setValue(
        getSliderPosition(highEqFreq,
                          SliderHiEQ->minimum(),
                          SliderHiEQ->maximum()));
    SliderLoEQ->setValue(
        getSliderPosition(lowEqFreq,
                          SliderLoEQ->minimum(),
                          SliderLoEQ->maximum()));

    CheckBoxLoFi->setChecked(m_pConfig->getValueString(
            ConfigKey(CONFIG_KEY, "LoFiEQs")) == QString("yes"));

    // Default internal EQs to enabled.
    CheckBoxEnbEQ->setChecked(m_pConfig->getValueString(
            ConfigKey(CONFIG_KEY, ENABLE_INTERNAL_EQ), "yes") == QString("yes"));
}

void DlgPrefEQ::setDefaultShelves()
{
    m_pConfig->set(ConfigKey(CONFIG_KEY, "HiEQFrequency"), ConfigValue(2500));
    m_pConfig->set(ConfigKey(CONFIG_KEY, "LoEQFrequency"), ConfigValue(250));
    m_pConfig->set(ConfigKey(CONFIG_KEY, "HiEQFrequencyPrecise"), ConfigValue(2500.0));
    m_pConfig->set(ConfigKey(CONFIG_KEY, "LoEQFrequencyPrecise"), ConfigValue(250.0));
}

void DlgPrefEQ::slotResetToDefaults() {
    setDefaultShelves();
    CheckBoxEnbEQ->setChecked(true);
    CheckBoxLoFi->setChecked(true);
    setDefaultShelves();
    loadSettings();
    slotUpdate();
    slotApply();
}

void DlgPrefEQ::slotEnaEQChanged() {
    m_pConfig->set(ConfigKey(CONFIG_KEY, ENABLE_INTERNAL_EQ),
                   CheckBoxEnbEQ->isChecked() ? QString("yes") : QString("no"));
}

void DlgPrefEQ::slotLoFiChanged()
{
    GroupBoxHiEQ->setEnabled(!CheckBoxLoFi->isChecked());
    GroupBoxLoEQ->setEnabled(!CheckBoxLoFi->isChecked());
    if(CheckBoxLoFi->isChecked()) {
        m_pConfig->set(ConfigKey(CONFIG_KEY, "LoFiEQs"), ConfigValue(QString("yes")));
    } else {
        m_pConfig->set(ConfigKey(CONFIG_KEY, "LoFiEQs"), ConfigValue(QString("no")));
    }
    slotApply();
}

void DlgPrefEQ::slotEffectChangedOnDeck(QString effectId) {
    QComboBox* c = qobject_cast<QComboBox*>(sender());
    // Check if qobject_cast was successful
    if (c) {
        int deckNumber = m_deckEffectSelectors.indexOf(c);
        emit(effectOnChainSlot(deckNumber, 0, effectId));

        // Update the configured effect for the current QComboBox
        m_pConfig->set(ConfigKey(CONFIG_KEY, QString("EffectForDeck%1").
                       arg(deckNumber + 1)), ConfigValue(effectId));
    }
}

void DlgPrefEQ::slotUpdateHiEQ()
{
    if (SliderHiEQ->value() < SliderLoEQ->value())
    {
        SliderHiEQ->setValue(SliderLoEQ->value());
    }
    m_highEqFreq = getEqFreq(SliderHiEQ->value(),
                             SliderHiEQ->minimum(),
                             SliderHiEQ->maximum());
    validate_levels();
    if (m_highEqFreq < 1000) {
        TextHiEQ->setText( QString("%1 Hz").arg((int)m_highEqFreq));
    } else {
        TextHiEQ->setText( QString("%1 kHz").arg((int)m_highEqFreq / 1000.));
    }
    m_pConfig->set(ConfigKey(CONFIG_KEY, "HiEQFrequency"),
                   ConfigValue(QString::number(static_cast<int>(m_highEqFreq))));
    m_pConfig->set(ConfigKey(CONFIG_KEY, "HiEQFrequencyPrecise"),
                   ConfigValue(QString::number(m_highEqFreq, 'f')));

    slotApply();
}

void DlgPrefEQ::slotUpdateLoEQ()
{
    if (SliderLoEQ->value() > SliderHiEQ->value())
    {
        SliderLoEQ->setValue(SliderHiEQ->value());
    }
    m_lowEqFreq = getEqFreq(SliderLoEQ->value(),
                            SliderLoEQ->minimum(),
                            SliderLoEQ->maximum());
    validate_levels();
    if (m_lowEqFreq < 1000) {
        TextLoEQ->setText(QString("%1 Hz").arg((int)m_lowEqFreq));
    } else {
        TextLoEQ->setText(QString("%1 kHz").arg((int)m_lowEqFreq / 1000.));
    }
    m_pConfig->set(ConfigKey(CONFIG_KEY, "LoEQFrequency"),
                   ConfigValue(QString::number(static_cast<int>(m_lowEqFreq))));
    m_pConfig->set(ConfigKey(CONFIG_KEY, "LoEQFrequencyPrecise"),
                   ConfigValue(QString::number(m_lowEqFreq, 'f')));

    slotApply();
}

int DlgPrefEQ::getSliderPosition(double eqFreq, int minValue, int maxValue)
{
    if (eqFreq >= kFrequencyUpperLimit) {
        return maxValue;
    } else if (eqFreq <= kFrequencyLowerLimit) {
        return minValue;
    }
    double dsliderPos = (eqFreq - kFrequencyLowerLimit) / (kFrequencyUpperLimit-kFrequencyLowerLimit);
    dsliderPos = pow(dsliderPos, 1.0 / 4.0) * (maxValue - minValue) + minValue;
    return dsliderPos;
}


void DlgPrefEQ::slotApply()
{
    m_COTLoFreq.slotSet(m_lowEqFreq);
    m_COTHiFreq.slotSet(m_highEqFreq);
    m_COTLoFi.slotSet(CheckBoxLoFi->isChecked());
    m_COTEnableEq.slotSet(CheckBoxEnbEQ->isChecked());
}

void DlgPrefEQ::slotUpdate()
{
    slotUpdateLoEQ();
    slotUpdateHiEQ();
    slotLoFiChanged();
    slotEnaEQChanged();
}

double DlgPrefEQ::getEqFreq(int sliderVal, int minValue, int maxValue) {
    // We're mapping f(x) = x^4 onto the range kFrequencyLowerLimit,
    // kFrequencyUpperLimit with x [minValue, maxValue]. First translate x into
    // [0.0, 1.0], raise it to the 4th power, and then scale the result from
    // [0.0, 1.0] to [kFrequencyLowerLimit, kFrequencyUpperLimit].
    double normValue = static_cast<double>(sliderVal - minValue) /
            (maxValue - minValue);
    // Use a non-linear mapping between slider and frequency.
    normValue = normValue * normValue * normValue * normValue;
    double result = normValue * (kFrequencyUpperLimit-kFrequencyLowerLimit) +
            kFrequencyLowerLimit;
    return result;
}

void DlgPrefEQ::validate_levels() {
    m_highEqFreq = math_clamp<double>(m_highEqFreq, kFrequencyLowerLimit,
                                      kFrequencyUpperLimit);
    m_lowEqFreq = math_clamp<double>(m_lowEqFreq, kFrequencyLowerLimit,
                                     kFrequencyUpperLimit);
    if (m_lowEqFreq == m_highEqFreq) {
        if (m_lowEqFreq == kFrequencyLowerLimit) {
            ++m_highEqFreq;
        } else if (m_highEqFreq == kFrequencyUpperLimit) {
            --m_lowEqFreq;
        } else {
            ++m_highEqFreq;
        }
    }
}
