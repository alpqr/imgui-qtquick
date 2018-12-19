Integration of [Dear ImGui](https://github.com/ocornut/imgui) 1.66b with [Qt Quick](http://doc.qt.io/qt-5/qtquick-index.html)

There are multiple ways to do this. Here we chose to expose a QQuickItem backed
by a QSGRenderNode (that issues the draw calls directly, no intermediate render
target; both threaded and basic render loops should work).

Alternatives would be to do it overlay-style connected to
QQuickWindow::afterRendering() or to construct scenegraph nodes (a geometry
node for each ImDrawCmd, clip nodes, etc.), each coming with its own pros and
cons.

By having a true Quick item, the ImGui "desktop"'s position, size, stacking,
opacity, etc. is fully under the application's control and can be controlled
via the usual QML means, while the frame generation can be hooked onto a
convenient signal. Note that this will not allow arbitrary transformations on
the item, apart from translation (scale should be fixed at some point though.
the rest just won't work.)

```
import QtQuick 2.0
import ImGui 1.0

Rectangle {
    id: root

    ...

    ImGui {
        objectName: "imgui"
        anchors.fill: parent
        focus: true
    }
}

int main(int argc, char *argv[])
{
    ...

    QQuickView viewer;
    viewer.setSource(QUrl("qrc:/main.qml"));

    ImGuiItem *imguiItem = viewer.rootObject()->findChild<ImGuiItem *>("imgui");
    QObject::connect(imguiItem, &ImGuiItem::frame, imguiItem, [] {
        ImGui::Text("Hello, world!");
    });

    ...
```
