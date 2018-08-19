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

#ifndef IMGUIITEM_H
#define IMGUIITEM_H

#include <QQuickItem>
#include <QImage>
#include <QSGRenderNode>

struct ImGuiContext;
class QOpenGLTexture;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class ImGuiInputEventFilter;

class ImGuiRenderer : public QSGRenderNode
{
public:
    ImGuiRenderer();
    ~ImGuiRenderer();

    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;

    struct FrameDesc {
        QVector<QImage> textures;
        struct Cmd {
            uint elemCount;
            const void *indexOffset;
            QPointF scissorPixelBottomLeft;
            QSizeF scissorPixelSize;
            uint textureIndex;
        };
        struct CmdListEntry {
            QByteArray vbuf;
            QByteArray ibuf;
            QVector<Cmd> cmds;
        };
        QVector<CmdListEntry> cmdList;
    };

private:
    QPointF m_scenePixelPosBottomLeft;
    QSizeF m_itemPixelSize;
    QSizeF m_itemSize;
    float m_dpr;
    FrameDesc m_frameDesc;
    QVector<QOpenGLTexture *> m_textures;
    QOpenGLShaderProgram *m_program = nullptr;
    int m_mvpLoc;
    int m_texLoc;
    int m_opacityLoc;
    QOpenGLVertexArrayObject *m_vao = nullptr;
    QOpenGLBuffer *m_vbo = nullptr;
    QOpenGLBuffer *m_ibo = nullptr;

    friend class ImGuiItem;
};

class ImGuiItem : public QQuickItem
{
    Q_OBJECT

public:
    ImGuiItem(QQuickItem *parent = nullptr);
    ~ImGuiItem();

signals:
    void frame();

private:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void itemChange(ItemChange, const ItemChangeData &) override;
    void updatePolish() override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void hoverMoveEvent(QHoverEvent *event) override;
    void touchEvent(QTouchEvent *event) override;

    void initialize();
    void cleanup();

    void setInputEventSource(QObject *src);
    void updateInput();

    QQuickWindow *m_w = nullptr;
    qreal m_dpr;
    QMetaObject::Connection m_c;
    ImGuiContext *m_imGuiContext = nullptr;
    ImGuiRenderer::FrameDesc m_frameDesc;
    bool m_inputInitialized = false;
    ImGuiInputEventFilter *m_inputEventFilter = nullptr;
    QObject *m_inputEventSource = nullptr;
    QObject m_dummy;
};

#endif
