#include "svg.h"

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point){
    points_.emplace_back(point);
    
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
    auto& out = context.out;
    
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (const auto point : points_){
        if(!is_first){
            out << " "sv;
        }
        
        out << point.x << ","sv << point.y;
        is_first = false;
    }
    out << "\"";
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    
    return *this;
}

Text& Text::SetFontSize(uint32_t size){
    size_ = size;
    
    return *this;
}

Text& Text::SetFontFamily(std::string font_family){
    font_family_ = std::move(font_family);
    
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight){
    font_weight_ = std::move(font_weight);
    
    return *this;
}

Text& Text::SetData(std::string data){
    data_ = std::move(data);
    
    return *this;
}

void Text::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    
    out << "<text";
    RenderAttrs(context.out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" font-size=\"" << size_;
    
    if(!font_family_.empty()){
        out << "\" font-family=\""sv << font_family_;
    }
    
    if(!font_weight_.empty()){
        out << "\" font-weight=\""sv << font_weight_;
    }
    
    out << "\">"sv;
    for (auto ch : data_){
        if (ch == '"'){
            out << "&quot;";
        }
        else if (ch == '\''){
            out << "&apos;";
        }
        else if (ch == '<'){
            out << "&lt;";
        }
        else if (ch == '>'){
            out << "&gt;";
        }
        else if (ch == '&'){
            out << "&amp;";
        }
        else{
            out << ch;
        }
    }
    out << "</text>";
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    for (const auto& obj : objects_) {
        out << "  "sv;
        obj->Render(out);
    }
    out << "</svg>";
}

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        default:
            break;
    }
    
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join) {
    switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
        default:
            break;
    }
    
    return out;
}

std::ostream& operator<<(std::ostream& out, const Color color) {
    std::visit(ColorPrinter{out}, color);
    
    return out;
}

} // end of namespace svg