/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imguiitem.h"
#include <QQuickWindow>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QGuiApplication>
#include <imgui.h>

ImGuiRenderer::ImGuiRenderer()
{
}

ImGuiRenderer::~ImGuiRenderer()
{
    releaseResources();
}

void ImGuiRenderer::releaseResources()
{
    qDeleteAll(m_textures);
    m_textures.clear();

    delete m_program;
    m_program = nullptr;

    delete m_vbo;
    m_vbo = nullptr;

    delete m_ibo;
    m_ibo = nullptr;

    delete m_vao;
    m_vao = nullptr;
}

static const char *vertSrcES2 =
        "attribute vec4 vertexPosition;\n"
        "attribute vec2 vertexTexCoord;\n"
        "attribute vec4 vertexColor;\n"
        "varying vec2 uv;\n"
        "varying vec4 color;\n"
        "uniform mat4 mvp;\n"
        "void main() {\n"
        "    uv = vertexTexCoord;\n"
        "    color = vertexColor;\n"
        "    gl_Position = mvp * vec4(vertexPosition.xy, 0.0, 1.0);\n"
        "}\n";

static const char *fragSrcES2 =
        "uniform sampler2D tex;\n"
        "uniform lowp float opacity;\n"
        "varying highp vec2 uv;\n"
        "varying highp vec4 color;\n"
        "void main() {\n"
        "    highp vec4 c = color * texture2D(tex, uv);\n"
        "    gl_FragColor = vec4(c.rgb, c.a * opacity);\n"
        "}\n";

static const char *vertSrcGL3 =
        "#version 150\n"
        "in vec4 vertexPosition;\n"
        "in vec2 vertexTexCoord;\n"
        "in vec4 vertexColor;\n"
        "out vec2 uv;\n"
        "out vec4 color;\n"
        "uniform mat4 mvp;\n"
        "void main() {\n"
        "    uv = vertexTexCoord;\n"
        "    color = vertexColor;\n"
        "    gl_Position = mvp * vec4(vertexPosition.xy, 0.0, 1.0);\n"
        "}\n";

static const char *fragSrcGL3 =
        "#version 150\n"
        "uniform sampler2D tex;\n"
        "uniform float opacity;\n"
        "in vec2 uv;\n"
        "in vec4 color;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    highp vec4 c = color * texture(tex, uv);\n"
        "    fragColor = vec4(c.rgb, c.a * opacity);\n"
        "}\n";

void ImGuiRenderer::render(const RenderState *state)
{
    Q_ASSERT(sizeof(ImDrawIdx) == 2);
    Q_ASSERT(m_frameDesc.textures.count() >= m_textures.count());

    // any new textures?
    for (int i = m_textures.count(); i < m_frameDesc.textures.count(); ++i) {
        QOpenGLTexture *t = new QOpenGLTexture(m_frameDesc.textures[i]);
        m_textures.append(t);
    }

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    const bool isCoreProfile  = ctx->format().profile() == QSurfaceFormat::CoreProfile;
    QOpenGLFunctions *f = ctx->functions();

    auto setupVertAttrs = [this, f] {
        m_vbo->bind();
        f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), 0);
        f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *) sizeof(ImVec2));
        f->glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *) (2 * sizeof(ImVec2)));
        f->glEnableVertexAttribArray(0);
        f->glEnableVertexAttribArray(1);
        f->glEnableVertexAttribArray(2);
    };

    if (!m_program) {
        m_program = new QOpenGLShaderProgram;
        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, isCoreProfile ? vertSrcGL3 : vertSrcES2);
        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, isCoreProfile ? fragSrcGL3 : fragSrcES2);
        m_program->bindAttributeLocation("vertexPosition", 0);
        m_program->bindAttributeLocation("vertexTexCoord", 1);
        m_program->bindAttributeLocation("vertexColor", 2);
        m_program->link();

        m_mvpLoc = m_program->uniformLocation("mvp");
        m_texLoc = m_program->uniformLocation("tex");
        m_opacityLoc = m_program->uniformLocation("opacity");

        m_program->setUniformValue(m_texLoc, 0);

        m_vao = new QOpenGLVertexArrayObject;
        m_vao->create();

        m_vbo = new QOpenGLBuffer;
        m_vbo->create();

        m_ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_ibo->create();
    }

    // non-premultiplied alpha
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // no backface culling
    f->glDisable(GL_CULL_FACE);
    // still need depth test to test against the items rendered in the opaque pass
    f->glEnable(GL_DEPTH_TEST);
    // but no need to write out anything to the depth buffer
    f->glDepthMask(GL_FALSE);
    // do not write out alpha
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    // will always scissor
    f->glEnable(GL_SCISSOR_TEST);

    if (m_vao->isCreated())
        m_vao->bind();

    m_program->bind();
    QMatrix4x4 m = *state->projectionMatrix() * *matrix();
    m_program->setUniformValue(m_mvpLoc, m);
    m_program->setUniformValue(m_opacityLoc, float(inheritedOpacity()));

    for (const FrameDesc::CmdListEntry &e : m_frameDesc.cmdList) {
        m_vbo->bind();
        m_vbo->allocate(e.vbuf.constData(), e.vbuf.size());
        m_ibo->bind();
        m_ibo->allocate(e.ibuf.constData(), e.ibuf.size());

        setupVertAttrs();

        for (const FrameDesc::Cmd &cmd : e.cmds) {
            qreal sx = cmd.scissorPixelBottomLeft.x() + m_scenePixelPosBottomLeft.x();
            qreal sy = cmd.scissorPixelBottomLeft.y() + m_scenePixelPosBottomLeft.y();
            qreal sw = qMin(cmd.scissorPixelSize.width(), m_itemPixelSize.width());
            qreal sh = qMin(cmd.scissorPixelSize.height(), m_itemPixelSize.height());
            if (state->scissorEnabled()) { // when the item or an ancestor has clip: true
                const QRectF r = state->scissorRect(); // bottom-left already
                sx = qMax(sx, r.x());
                sy = qMax(sy, r.y());
                sw = qMin(sw, r.width());
                sh = qMin(sh, r.height());
            }
            f->glScissor(sx, sy, sw, sh);

            m_textures[cmd.textureIndex]->bind();

            f->glDrawElements(GL_TRIANGLES, cmd.elemCount, GL_UNSIGNED_SHORT, cmd.indexOffset);
        }
    }

    // restore this one, just in case; the others are reported from changedStates()
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

QSGRenderNode::StateFlags ImGuiRenderer::changedStates() const
{
    return ScissorState | BlendState | DepthState | CullState;
}

QSGRenderNode::RenderingFlags ImGuiRenderer::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF ImGuiRenderer::rect() const
{
    return QRect(0, 0, m_itemSize.width(), m_itemSize.height());
}

ImGuiItem::ImGuiItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
}

ImGuiItem::~ImGuiItem()
{
    cleanup();
}

QSGNode *ImGuiItem::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *)
{
    ImGuiRenderer *n = static_cast<ImGuiRenderer *>(node);
    if (!n)
        n = new ImGuiRenderer;

    // This is on the render thread with the main thread blocked. Synchronize the
    // data prepared in the polish step on the main thread.
    const QPointF sceneTopLeft = mapToScene(QPointF(0, 0));
    QQuickWindow *w = window();
    n->m_scenePixelPosBottomLeft = QPointF(sceneTopLeft.x(), w->height() - (sceneTopLeft.y() + height())) * m_dpr;
    n->m_itemPixelSize = size() * m_dpr;
    n->m_itemSize = size();
    n->m_dpr = m_dpr;
    n->m_frameDesc = m_frameDesc;

    n->markDirty(QSGNode::DirtyMaterial);

    return n;
}

void ImGuiItem::itemChange(QQuickItem::ItemChange change,
                           const QQuickItem::ItemChangeData &changeData)
{
    if (change == QQuickItem::ItemSceneChange) {
        cleanup();
        if (changeData.window)
            initialize();
    }
}

void ImGuiItem::initialize()
{
    m_w = window();

    m_imGuiContext = ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

    const QImage wrapperImg((const uchar *) pixels, w, h, QImage::Format_RGBA8888);
    m_frameDesc.textures.append(wrapperImg.copy());
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(quintptr(m_frameDesc.textures.count() - 1)));

    // the imgui "render loop"
    m_c = connect(m_w, &QQuickWindow::afterAnimating, m_w, [this] {
        polish();
    });

    setInputEventSource(&m_dummy);
}

#define FIRSTSPECKEY (0x01000000)
#define LASTSPECKEY (0x01000017)
#define MAPSPECKEY(k) ((k) - FIRSTSPECKEY + 256)

class ImGuiInputEventFilter : public QObject
{
public:
    ImGuiInputEventFilter()
    {
        memset(keyDown, 0, sizeof(keyDown));
    }

    bool eventFilter(QObject *watched, QEvent *event) override;

    QPointF mousePos;
    Qt::MouseButtons mouseButtonsDown = Qt::NoButton;
    float mouseWheel = 0;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    bool keyDown[256 + (LASTSPECKEY - FIRSTSPECKEY + 1)];
    QString keyText;
};

void ImGuiItem::cleanup()
{
    if (m_imGuiContext) {
        ImGui::DestroyContext();
        m_imGuiContext = nullptr;
    }

    if (m_w) {
        disconnect(m_c);
        m_w = nullptr;
    }

    delete m_inputEventFilter;
    m_inputEventFilter = nullptr;
}

void ImGuiItem::updatePolish()
{
    m_dpr = m_w->effectiveDevicePixelRatio();

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize.x = width();
    io.DisplaySize.y = height();
    io.DisplayFramebufferScale = ImVec2(m_dpr, m_dpr);

    updateInput();

    ImGui::NewFrame();
    emit frame();
    ImGui::Render();

    ImDrawData *d = ImGui::GetDrawData();
    d->ScaleClipRects(ImVec2(m_dpr, m_dpr)); // so cmd->ClipRect is now in pixels

    m_frameDesc.cmdList.clear();

    // CmdLists is in back-to-front order, this is good
    for (int n = 0; n < d->CmdListsCount; ++n) {
        const ImDrawList *cmdList = d->CmdLists[n];
        const ImDrawIdx *indexBufOffset = nullptr;
        ImGuiRenderer::FrameDesc::CmdListEntry e;

        e.vbuf = QByteArray((const char *) cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        e.ibuf = QByteArray((const char *) cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

        for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
            const ImDrawCmd *cmd = &cmdList->CmdBuffer[i];

            if (!cmd->UserCallback) {
                ImGuiRenderer::FrameDesc::Cmd qcmd;
                qcmd.elemCount = cmd->ElemCount;
                qcmd.indexOffset = indexBufOffset;

                qcmd.scissorPixelBottomLeft = QPointF(cmd->ClipRect.x, io.DisplaySize.y * m_dpr - cmd->ClipRect.w);
                qcmd.scissorPixelSize = QSizeF(cmd->ClipRect.z - cmd->ClipRect.x, cmd->ClipRect.w - cmd->ClipRect.y);

                qcmd.textureIndex = uint(reinterpret_cast<quintptr>(cmd->TextureId));

                e.cmds.append(qcmd);
            } else {
                cmd->UserCallback(cmdList, cmd);
            }

            indexBufOffset += cmd->ElemCount;
        }

        m_frameDesc.cmdList.append(e);
    }

    update();
}

void ImGuiItem::keyPressEvent(QKeyEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

void ImGuiItem::keyReleaseEvent(QKeyEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

void ImGuiItem::mousePressEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

void ImGuiItem::mouseMoveEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

void ImGuiItem::mouseReleaseEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

void ImGuiItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

#if QT_CONFIG(wheelevent)
void ImGuiItem::wheelEvent(QWheelEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}
#endif

void ImGuiItem::hoverMoveEvent(QHoverEvent *event)
{
    // Simulate the QWindow behavior, which means sending moves even when no
    // button is down.

    if (QGuiApplication::mouseButtons() != Qt::NoButton)
        return;

    const QPointF sceneOffset = mapToScene(event->pos());
    const QPointF globalOffset = mapToGlobal(event->pos());
    QMouseEvent e(QEvent::MouseMove, event->pos(), event->pos() + sceneOffset, event->pos() + globalOffset,
                  Qt::NoButton, Qt::NoButton, QGuiApplication::keyboardModifiers());
    QCoreApplication::sendEvent(&m_dummy, &e);
}

void ImGuiItem::touchEvent(QTouchEvent *event)
{
    QCoreApplication::sendEvent(&m_dummy, event);
}

bool ImGuiInputEventFilter::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        mousePos = me->pos();
        mouseButtonsDown = me->buttons();
        modifiers = me->modifiers();
    }
        break;

    case QEvent::Wheel:
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
        mouseWheel += we->angleDelta().y() / 120.0f;
    }
        break;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        const bool down = event->type() == QEvent::KeyPress;
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        modifiers = ke->modifiers();
        if (down)
            keyText.append(ke->text());
        int k = ke->key();
        if (k <= 0xFF)
            keyDown[k] = down;
        else if (k >= FIRSTSPECKEY && k <= LASTSPECKEY)
            keyDown[MAPSPECKEY(k)] = down;
    }
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *te = static_cast<QTouchEvent *>(event);
        mousePos = te->touchPoints().first().pos();
        mouseButtonsDown = te->touchPoints().first().state() == Qt::TouchPointReleased
                ? Qt::NoButton : Qt::LeftButton;
        modifiers = Qt::NoModifier;
    }
        break;

    default:
        break;
    }

    return false;
}

void ImGuiItem::setInputEventSource(QObject *src)
{
    if (m_inputEventSource && m_inputEventFilter)
        m_inputEventSource->removeEventFilter(m_inputEventFilter);

    m_inputEventSource = src;

    if (!m_inputEventFilter)
        m_inputEventFilter = new ImGuiInputEventFilter;

    m_inputEventSource->installEventFilter(m_inputEventFilter);
}

void ImGuiItem::updateInput()
{
    if (!m_inputEventFilter)
        return;

    ImGuiIO &io = ImGui::GetIO();

    if (!m_inputInitialized) {
        m_inputInitialized = true;

        io.KeyMap[ImGuiKey_Tab] = MAPSPECKEY(Qt::Key_Tab);
        io.KeyMap[ImGuiKey_LeftArrow] = MAPSPECKEY(Qt::Key_Left);
        io.KeyMap[ImGuiKey_RightArrow] = MAPSPECKEY(Qt::Key_Right);
        io.KeyMap[ImGuiKey_UpArrow] = MAPSPECKEY(Qt::Key_Up);
        io.KeyMap[ImGuiKey_DownArrow] = MAPSPECKEY(Qt::Key_Down);
        io.KeyMap[ImGuiKey_PageUp] = MAPSPECKEY(Qt::Key_PageUp);
        io.KeyMap[ImGuiKey_PageDown] = MAPSPECKEY(Qt::Key_PageDown);
        io.KeyMap[ImGuiKey_Home] = MAPSPECKEY(Qt::Key_Home);
        io.KeyMap[ImGuiKey_End] = MAPSPECKEY(Qt::Key_End);
        io.KeyMap[ImGuiKey_Delete] = MAPSPECKEY(Qt::Key_Delete);
        io.KeyMap[ImGuiKey_Backspace] = MAPSPECKEY(Qt::Key_Backspace);
        io.KeyMap[ImGuiKey_Enter] = MAPSPECKEY(Qt::Key_Return);
        io.KeyMap[ImGuiKey_Escape] = MAPSPECKEY(Qt::Key_Escape);

        io.KeyMap[ImGuiKey_A] = Qt::Key_A;
        io.KeyMap[ImGuiKey_C] = Qt::Key_C;
        io.KeyMap[ImGuiKey_V] = Qt::Key_V;
        io.KeyMap[ImGuiKey_X] = Qt::Key_X;
        io.KeyMap[ImGuiKey_Y] = Qt::Key_Y;
        io.KeyMap[ImGuiKey_Z] = Qt::Key_Z;
    }

    ImGuiInputEventFilter *w = m_inputEventFilter;

    io.MousePos = ImVec2(w->mousePos.x(), w->mousePos.y());

    io.MouseDown[0] = w->mouseButtonsDown.testFlag(Qt::LeftButton);
    io.MouseDown[1] = w->mouseButtonsDown.testFlag(Qt::RightButton);
    io.MouseDown[2] = w->mouseButtonsDown.testFlag(Qt::MiddleButton);

    io.MouseWheel = w->mouseWheel;
    w->mouseWheel = 0;

    io.KeyCtrl = w->modifiers.testFlag(Qt::ControlModifier);
    io.KeyShift = w->modifiers.testFlag(Qt::ShiftModifier);
    io.KeyAlt = w->modifiers.testFlag(Qt::AltModifier);
    io.KeySuper = w->modifiers.testFlag(Qt::MetaModifier);

    memcpy(io.KeysDown, w->keyDown, sizeof(w->keyDown));

    if (!w->keyText.isEmpty()) {
        for (const QChar &c : w->keyText) {
            ImWchar u = c.unicode();
            if (u)
                io.AddInputCharacter(u);
        }
        w->keyText.clear();
    }
}
