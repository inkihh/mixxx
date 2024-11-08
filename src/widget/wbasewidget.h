#pragma once

#include <QString>
#include <QWidget>
#include <memory>
#include <vector>

class ControlWidgetPropertyConnection;
class ControlParameterWidgetConnection;

class WBaseWidget {
  public:
    explicit WBaseWidget(QWidget* pWidget);
    virtual ~WBaseWidget();

    virtual void Init();

    QWidget* toQWidget() {
        return m_pWidget;
    }

    void appendBaseTooltip(const QString& tooltip) {
        m_baseTooltip.append(tooltip);
        m_pWidget->setToolTip(m_baseTooltip);
    }

    void prependBaseTooltip(const QString& tooltip) {
        m_baseTooltip.prepend(tooltip);
        m_pWidget->setToolTip(m_baseTooltip);
    }

    void setBaseTooltip(const QString& tooltip) {
        m_baseTooltip = tooltip;
        m_pWidget->setToolTip(tooltip);
    }

    QString baseTooltip() const {
        return m_baseTooltip;
    }

    void addLeftConnection(std::unique_ptr<ControlParameterWidgetConnection> pConnection);
    void addRightConnection(std::unique_ptr<ControlParameterWidgetConnection> pConnection);
    void addConnection(std::unique_ptr<ControlParameterWidgetConnection> pConnection);

    void addPropertyConnection(std::unique_ptr<ControlWidgetPropertyConnection> pConnection);

    // Set a ControlWidgetConnection to be the display connection for the
    // widget. The connection should also be added via an addConnection method
    // or it will not be deleted or receive updates.
    void setDisplayConnection(ControlParameterWidgetConnection* pConnection);

    double getControlParameter() const;
    double getControlParameterLeft() const;
    double getControlParameterRight() const;
    double getControlParameterDisplay() const;


  protected:
    // Whenever a connected control is changed, onConnectedControlChanged is
    // called. This allows the widget implementer to respond to the change and
    // gives them both the parameter and its corresponding value.
    virtual void onConnectedControlChanged(double dParameter, double dValue) {
        Q_UNUSED(dParameter);
        Q_UNUSED(dValue);
    }

    void resetControlParameter();
    void setControlParameter(double v);
    void setControlParameterDown(double v);
    void setControlParameterUp(double v);
    void setControlParameterLeftDown(double v);
    void setControlParameterLeftUp(double v);
    void setControlParameterRightDown(double v);
    void setControlParameterRightUp(double v);

    // Tooltip handling. We support "debug tooltips" which are basically a way
    // to expose debug information about widgets via the tooltip. To enable
    // this, when widgets should call updateTooltip before they are about to
    // display a tooltip.
    void updateTooltip();
    virtual void fillDebugTooltip(QStringList* debug);

    std::vector<std::unique_ptr<ControlParameterWidgetConnection>> m_connections;
    ControlParameterWidgetConnection* m_pDisplayConnection;
    std::vector<std::unique_ptr<ControlParameterWidgetConnection>> m_leftConnections;
    std::vector<std::unique_ptr<ControlParameterWidgetConnection>> m_rightConnections;

    std::vector<std::unique_ptr<ControlWidgetPropertyConnection>> m_propertyConnections;

  private:
    QWidget* m_pWidget;
    QString m_baseTooltip;

    friend class ControlParameterWidgetConnection;
};
