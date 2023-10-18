#include "svg.h"

#include <iomanip>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << std::setprecision(6) << center_.x
        << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << std::setprecision(6) << radius_ << "\""sv;

    RenderAttrs(out);

    out << "/>"sv;
}

// ---------- Polyline ------------------
Polyline& Polyline::AddPoint(svg::Point point) {
    points_.emplace_back(std::move(point));

    return *this;
}

void Polyline::RenderObject(const svg::RenderContext& context) const {
    auto& out = context.out;

    out << "<polyline points=\""sv;
    for (auto iter = points_.begin(); iter != points_.end(); ++iter) {
        auto [x, y] = *iter;
        out << std::setprecision(6) << x << ","sv << y;
        if (std::next(iter) != points_.end()) {
            out << " "sv;
        }
    }
    out << "\""sv;

    RenderAttrs(out);

    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    position_ = pos;

    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;

    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;

    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;

    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;

    return *this;
}

Text& Text::SetData(std::string data) {
    text_data_ = data;

    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;

    RenderAttrs(out);

    // text position and offset
    out << std::setprecision(6) << " x=\""sv << position_.x << "\" y=\""sv
        << position_.y << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y
        << "\""sv;

    // font size
    out << " font-size=\""sv << font_size_ << "\""sv;

    // font family
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }

    // font weight
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }

    // end of text tag
    out << ">"sv;

    for (char c : text_data_) {
        out << CheckAndScreenCharacter(c);
    }

    out << "</text>"sv;
}

std::string Text::CheckAndScreenCharacter(char c) const {
    std::string out_c;

    switch (c) {
        case '\"':
            out_c = "&quot;"s;
            break;
        case '\'':
            out_c = "&apos;"s;
            break;
        case '<':
            out_c = "&lt;"s;
            break;
        case '>':
            out_c = "&gt;"s;
            break;
        case '&':
            out_c = "&amp;"s;
            break;
        default:
            out_c = c;
    }

    return out_c;
}

// ---------- Document ------------------
void Document::Render(std::ostream& out) const {
    RenderContext context(out, 2, 2);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv
        << std::endl;

    for (auto iter = objects_ptrs_.begin(); iter != objects_ptrs_.end();
         ++iter) {
        (*iter)->Render(context);
    }

    out << "</svg>"sv;
}

// ---------- StrokeLineCap ------------------
std::ostream& operator<<(std::ostream& out, const StrokeLineCap& slc) {
    using namespace std::literals;
    switch (slc) {
        case StrokeLineCap::BUTT:
            out << "butt"s;
            break;
        case StrokeLineCap::ROUND:
            out << "round"s;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"s;
            break;
    }

    return out;
}

// ---------- StrokeLineJoin ------------------
std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& slj) {
    using namespace std::literals;
    switch (slj) {
        case StrokeLineJoin::ARCS:
            out << "arcs"s;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"s;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"s;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"s;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"s;
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const Rgb& color) {
    using namespace std::literals;
    out << "rgb("s << std::to_string(color.red) << ","s
        << std::to_string(color.green) << ","s << std::to_string(color.blue)
        << ")"s;
    return out;
}

std::ostream& operator<<(std::ostream& out, const Rgba& color) {
    using namespace std::literals;
    out << "rgba("s << std::to_string(color.red) << ","s
        << std::to_string(color.green) << ","s << std::to_string(color.blue)
        << ","s << color.opacity << ")"s;
    return out;
}

std::ostream& operator<<(std::ostream& out, const Color& color) {
    using namespace std::literals;

    std::visit(
        [&out](Color value) {
            if (std::holds_alternative<std::monostate>(value)) {
                out << NoneColor;
            } else if (const auto* val1 = std::get_if<std::string>(&value)) {
                out << *val1;
            } else if (const auto* val2 = std::get_if<Rgb>(&value)) {
                out << *val2;
            } else if (const auto* val3 = std::get_if<Rgba>(&value)) {
                out << *val3;
            }
        },
        color);

    return out;
}

}  // namespace svg