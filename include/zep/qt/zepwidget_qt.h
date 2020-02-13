#pragma once

#include <QTimer>
#include <QWidget>
#include <memory>

#include <QKeyEvent>
#include <QDesktopWidget>
#include <QDebug>

#include "zep/editor.h"
#include "zep/mode.h"
#include "zep/mode_standard.h"
#include "zep/mode_vim.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include "zep/qt/zepdisplay_qt.h"

namespace Zep
{

class ZepWidget_Qt : public QWidget, public IZepComponent
{
public:
    ZepWidget_Qt(QWidget* pParent, const ZepPath& root)
        : QWidget(pParent)
    {
        m_spEditor = std::make_unique<ZepEditor>(new ZepDisplay_Qt(), root);
        m_spEditor->RegisterCallback(this);
        m_spEditor->SetPixelScale(float(qApp->desktop()->logicalDpiX() / 96.0f));

        setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        setMouseTracking(true);

        m_refreshTimer.setInterval(1);
        m_refreshTimer.setSingleShot(false);
        m_refreshTimer.start();
        connect(&m_refreshTimer, &QTimer::timeout, this, &ZepWidget_Qt::OnTimer);
    }

    ~ZepWidget_Qt()
    {
        m_spEditor->UnRegisterCallback(this);

        m_spEditor.reset();
    }

    void Notify(std::shared_ptr<ZepMessage> message)
    {
        if (message->messageId == Msg::RequestQuit)
        {
            qApp->quit();
        }
    }

    void resizeEvent(QResizeEvent* pResize)
    {
        m_spEditor->SetDisplayRegion(NVec2f(0.0f, 0.0f), NVec2f(pResize->size().width(), pResize->size().height()));
    }

    void paintEvent(QPaintEvent* pPaint)
    {
        (void)pPaint;

        QPainter painter(this);
        painter.setRenderHint(QPainter::RenderHint::Antialiasing, true);
        painter.fillRect(rect(), QColor::fromRgbF(.1f, .1f, .1f, 1.0f));

        ((ZepDisplay_Qt&)m_spEditor->GetDisplay()).SetPainter(&painter);

        m_spEditor->Display();

        ((ZepDisplay_Qt&)m_spEditor->GetDisplay()).SetPainter(nullptr);
    }

    void keyPressEvent(QKeyEvent* ev)
    {
        uint32_t mod = 0;
        auto pMode = m_spEditor->GetActiveTabWindow()->GetActiveWindow()->GetBuffer().GetMode();

        if (ev->modifiers() & Qt::ShiftModifier)
        {
            mod |= ModifierKey::Shift;
        }
        if (ev->modifiers() & Qt::ControlModifier)
        {
            mod |= ModifierKey::Ctrl;
            if (ev->key() == Qt::Key_1)
            {
                m_spEditor->SetGlobalMode(ZepMode_Standard::StaticName());
                update();
                return;
            }
            else if (ev->key() == Qt::Key_2)
            {
                m_spEditor->SetGlobalMode(ZepMode_Vim::StaticName());
                update();
                return;
            }
            else
            {
                if (ev->key() >= Qt::Key_A && ev->key() <= Qt::Key_Z)
                {
                    pMode->AddKeyPress((ev->key() - Qt::Key_A) + 'a', mod);
                    update();
                    return;
                }
            }
        }

        if (ev->key() == Qt::Key_Tab)
        {
            pMode->AddKeyPress(ExtKeys::TAB, mod);
        }
        else if (ev->key() == Qt::Key_Escape)
        {
            pMode->AddKeyPress(ExtKeys::ESCAPE, mod);
        }
        else if (ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return)
        {
            pMode->AddKeyPress(ExtKeys::RETURN, mod);
        }
        else if (ev->key() == Qt::Key_Delete)
        {
            pMode->AddKeyPress(ExtKeys::DEL, mod);
        }
        else if (ev->key() == Qt::Key_Backspace)
        {
            pMode->AddKeyPress(ExtKeys::BACKSPACE, mod);
        }
        else if (ev->key() == Qt::Key_Home)
        {
            pMode->AddKeyPress(ExtKeys::HOME, mod);
        }
        else if (ev->key() == Qt::Key_End)
        {
            pMode->AddKeyPress(ExtKeys::END, mod);
        }
        else if (ev->key() == Qt::Key_Right)
        {
            pMode->AddKeyPress(ExtKeys::RIGHT, mod);
        }
        else if (ev->key() == Qt::Key_Left)
        {
            pMode->AddKeyPress(ExtKeys::LEFT, mod);
        }
        else if (ev->key() == Qt::Key_Up)
        {
            pMode->AddKeyPress(ExtKeys::UP, mod);
        }
        else if (ev->key() == Qt::Key_Down)
        {
            pMode->AddKeyPress(ExtKeys::DOWN, mod);
        }
        else
        {
            auto input = ev->text().toUtf8();
            if (!input.isEmpty())
            {
                uint8_t* pIn = (uint8_t*)input.data();

                // Convert to UTF8 DWORD; not used yet
                uint32_t dw = 0;
                for (int i = 0; i < input.size(); i++)
                {
                    dw |= ((DWORD)pIn[i]) << ((input.size() - i - 1) * 8);
                }

                pMode->AddKeyPress(dw, mod);
            }
        }
        update();
    }

    ZepMouseButton GetMouseButton(QMouseEvent* ev)
    {
        switch (ev->button())
        {
        case Qt::MouseButton::MiddleButton:
            return ZepMouseButton::Middle;
            break;
        case Qt::MouseButton::LeftButton:
            return ZepMouseButton::Left;
            break;
        case Qt::MouseButton::RightButton:
            return ZepMouseButton::Right;
        default:
            return ZepMouseButton::Unknown;
            break;
        }
    }

    void mousePressEvent(QMouseEvent* ev)
    {
        if (m_spEditor)
        {
            m_spEditor->OnMouseDown(toNVec2f(ev->localPos()), GetMouseButton(ev));
        }
    }
    void mouseReleaseEvent(QMouseEvent* ev)
    {
        (void)ev;
        if (m_spEditor)
        {
            m_spEditor->OnMouseUp(toNVec2f(ev->localPos()), GetMouseButton(ev));
        }
    }

    void mouseDoubleClickEvent(QMouseEvent* event)
    {
        (void)event;
    }

    void mouseMoveEvent(QMouseEvent* ev)
    {
        if (m_spEditor)
        {
            m_spEditor->OnMouseMove(toNVec2f(ev->localPos()));
        }
    }

    // IZepComponent
    virtual ZepEditor& GetEditor() const override
    {
        return *m_spEditor;
    }

private slots:
    void OnTimer()
    {
        if (m_spEditor->RefreshRequired())
        {
            update();
        }
    }


private:
    std::unique_ptr<ZepEditor> m_spEditor;
    QTimer m_refreshTimer;
};

} // namespace Zep
