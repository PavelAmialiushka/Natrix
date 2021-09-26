#include "sceneWidgetDrawer.h"

#include "geometry/nodePointAdorner.h"

#if 0
SceneWidgetDrawer::SceneWidgetDrawer(QPainter& painter, QRect windowRect)
    : painter_(&painter)
    , currentStyle_(NoStyle)
{
    // установка тощины линий
    changeScale(1.0);
    font_.fromString( fontString() );

    windowPath_.addRect(windowRect.adjusted(-1,-1,1,1));
    painter.setClipPath( windowPath_ );
    painter.setClipping(true);
}

QString SceneWidgetDrawer::fontString()
{
    QFont font("Tahoma", 12);
    font.setKerning(0);
    //font.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    return font.toString();
}

void SceneWidgetDrawer::exportPage(Scene* scene, QPaintDevice* printer, QRect windowRect)
{
    point2d lt, rb; std::tie(lt, rb) = scene->canvasRectangle();
    point2d size = rb-lt;
    QRect modelRect(lt.x, lt.y, abs(size.x), abs(size.y));

    // сохраняем пропорции
    double sls = qMin(windowRect.width() * 1.0 / modelRect.width(),
                      windowRect.height() * 1.0 / modelRect.height());
    QSize windowSize{static_cast<int>(modelRect.width() * sls),
                     static_cast<int>(modelRect.height() * sls)};
    windowRect = QRect(
                windowRect.center() - QPoint(windowSize.width(), windowSize.height()) / 2.0,
                windowSize);

    // настройка координат
    QPainter painter(printer);
    painter.setViewport(windowRect.left(), windowRect.top(), windowRect.width(), windowRect.height());
    painter.setWindow(modelRect.left(), modelRect.top(), modelRect.width(), modelRect.height());
    painter.setViewTransformEnabled(1);

    // заполнение фона
    painter.setBackground(QBrush(Qt::white));

    SceneWidgetDrawer(painter, modelRect)
            .redraw(scene, modelRect.width() * 1.0 / windowRect.width(), true);
}

const int SELECTED_FLAG=128;
void SceneWidgetDrawer::changeScale(double s)
{
    // курсор
    int scale = qMax(1.0, 0.5/s);
    int cursorSize = 1.5 / s;
    pens_[StyleAdornerCursorLine] = QPen(QBrush(QColor(255,0,0,63)), cursorSize,
                                         Qt::DashLine, Qt::RoundCap);

    // объекты
    int extraThickSize = 4 * scale;
    int normalSize = 2 * scale;
    int thinSize = 1 * scale;

    pens_[StyleLine] = QPen(QBrush(Qt::black), normalSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleElement] = QPen(QBrush(Qt::black), thinSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleDashDot] = QPen(QBrush(Qt::black), thinSize, Qt::DashDotLine, Qt::RoundCap);
    pens_[StyleBlackFilled] = QPen(QBrush(Qt::black), thinSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleViewport] = QPen(QBrush(Qt::gray), extraThickSize, Qt::SolidLine, Qt::RoundCap);

    int sel = SELECTED_FLAG;
    pens_[sel + StyleLine] = QPen(QBrush(Qt::blue), normalSize, Qt::SolidLine, Qt::RoundCap);
    pens_[sel + StyleElement] = QPen(QBrush(Qt::blue), thinSize, Qt::SolidLine, Qt::RoundCap);
    pens_[sel + StyleDashDot] = QPen(QBrush(Qt::blue), thinSize, Qt::DashDotLine, Qt::RoundCap);
    pens_[sel + StyleBlackFilled] = QPen(QBrush(Qt::blue), thinSize, Qt::SolidLine, Qt::RoundCap);
    pens_[sel + StyleViewport] = QPen(QBrush(Qt::blue), extraThickSize, Qt::SolidLine, Qt::RoundCap);

    pens_[StyleText] = QPen(QBrush(Qt::black), normalSize, Qt::SolidLine);
    pens_[PrivateStyleTextHover] = QPen(QBrush(Qt::red), 1, Qt::DotLine);

    // вспомогательная линия
    pens_[StyleStraightLineAdorner] = QPen(QBrush(Qt::red), thinSize, Qt::DotLine, Qt::RoundCap);

    // призрачные линии
    const QColor lightBlue = QColor(108,126,255);
    pens_[StyleGhostAdorner] = QPen(QBrush(lightBlue), thinSize, Qt::DotLine, Qt::RoundCap);

    // подсветка
    int hoverSize =  4 * scale;
    pens_[PrivateStyleHover] = QPen(QBrush(QColor(255,235,128)), hoverSize, Qt::SolidLine, Qt::RoundCap);
    pens_[PrivateStyleNewby] = QPen(QBrush(QColor(255,0,0)), hoverSize, Qt::SolidLine, Qt::RoundCap);

    const QColor lineGreen = QColor(21,179,0);

    // подсказки
    int boxSize = 2 * scale;
    pens_[StyleNodePointAdorner + AdornerLineFromNone] = QPen(QBrush(Qt::red), boxSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerLineFromNode] = QPen(QBrush(Qt::blue), boxSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerLineFromFree] = QPen(QBrush(Qt::black), boxSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerLineFromLine] = QPen(QBrush(lineGreen), boxSize, Qt::SolidLine, Qt::RoundCap);

    // перемещение
    pens_[StyleMoveLineAdorner] = QPen(QBrush(Qt::green), thinSize, Qt::DotLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerMovingStart]  = QPen(QBrush(Qt::red), boxSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerMovingToFree] = QPen(QBrush(Qt::red), boxSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerMovingToNode] = QPen(QBrush(Qt::blue), boxSize, Qt::SolidLine, Qt::RoundCap);
    pens_[StyleNodePointAdorner + AdornerMovingToLine] = QPen(QBrush(lineGreen), boxSize, Qt::SolidLine, Qt::RoundCap);

}

void SceneWidgetDrawer::setPen(int style)
{
    if (style != currentStyle_)
    {
        currentStyle_ = style;
        painter_->setPen(
                    pens_.count(style)
                    ? pens_.value(style)
                    : pens_.value(StyleLine));
    }
}

void SceneWidgetDrawer::prepare_draw(int style, int hover, int selected, function<void()> drawPrimitives)
{
    // если под мышкой
    if (hover && (style == StyleLine || style == StyleElement
                  || style == StyleDashDot))
    {
        setPen(hover == HoverState::Newby ? PrivateStyleNewby : PrivateStyleHover);
        drawPrimitives();
    }
    else if (style==StyleBlackFilled)
    {
        setPen( style );
        QBrush store = painter_->brush();
        painter_->setBrush( painter_->pen().color() );
        drawPrimitives();
        painter_->setBrush( store );
    }
    else
    {
        // выбираем стиль линии
        setPen( selected ? style + SELECTED_FLAG: style );
        drawPrimitives();
    }
}

void SceneWidgetDrawer::draw(PrimitiveLine const &line)
{
    prepare_draw(line.style, line.hover, line.selected,
                 bind(&SceneWidgetDrawer::simple_draw_line, this, line) );
}

void SceneWidgetDrawer::simple_draw_line(PrimitiveLine const &line)
{
    // рисуем линию
    painter_->drawLine(
            QPointF(line.start.x, line.start.y),
            QPointF(line.end.x, line.end.y));
}

void SceneWidgetDrawer::draw(PrimitiveBox const& box)
{
    prepare_draw(box.style, box.hover, box.selected,
                 bind(&SceneWidgetDrawer::simple_draw_box, this, box));
}

void SceneWidgetDrawer::simple_draw_box(PrimitiveBox const &box)
{
    painter_->setOpacity(0.5);

    painter_->drawRect(box.start.x - adornerBoxSize/2,
                      box.start.y - adornerBoxSize/2,
                      adornerBoxSize+1, adornerBoxSize+1);
    painter_->setOpacity(1);
}

void SceneWidgetDrawer::draw(PrimitiveCircle const& circle)
{
    prepare_draw(circle.style, circle.hover, circle.selected,
                 bind(&SceneWidgetDrawer::simple_draw_circle, this, circle.center, circle.radius));
}

void SceneWidgetDrawer::simple_draw_circle(point2d center, double radius)
{
    painter_->drawEllipse(QPointF(center.x, center.y), radius, radius);
}

void SceneWidgetDrawer::draw(PrimitiveSpline const& spline)
{
    prepare_draw(spline.style, spline.hover, spline.selected,
                 bind(&SceneWidgetDrawer::simple_draw_spline, this, spline.points));
}

void SceneWidgetDrawer::simple_draw_spline(QList<point2d> points)
{
    QPainterPath path;
    path.moveTo(points[0].x, points[0].y);
    for(int index=1; index < points.size(); ++index)
    {
        path.lineTo(points[index].x, points[index].y);
    }
    painter_->drawPath(path);
}

void SceneWidgetDrawer::draw(PrimitiveSpaceOn const& space)
{
    QPainterPath path;
    path.moveTo(space.points.last().x, space.points.last().y);
    for(int index=0; index < space.points.size(); ++index)
        path.lineTo(space.points[index].x, space.points[index].y);

    path = windowPath_ - path;

    painter_->setClipPath(path, Qt::IntersectClip);
    painter_->setClipping(true);
}

void SceneWidgetDrawer::draw(PrimitiveSpaceOff const&)
{
    painter_->setClipPath( windowPath_ );
    painter_->setClipping(true);
}

void SceneWidgetDrawer::draw(PrimitiveText const& text)
{
    QPoint lt(text.info.a.x, text.info.a.y);
    QRect rect(text.info.a.x,
               text.info.a.y,
               text.info.width,
               text.info.height);

    font_.setPixelSize(text.info.height);
    painter_->setFont(font_);

    setPen( text.hover ? PrivateStyleTextHover : StyleText);
    painter_->drawText(rect.bottomLeft(), text.info.text);
}

#endif
