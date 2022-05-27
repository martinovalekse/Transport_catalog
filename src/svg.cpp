#include "svg.h"

using namespace std::literals;

svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
	using namespace svg;
	Polyline polyline;
	for (int i = 0; i <= num_rays; ++i) {
		double angle = 2 * M_PI * (i % num_rays) / num_rays;
		polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
		if (i == num_rays) {
			break;
		}
		angle += M_PI / num_rays;
		polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
	}
	polyline.SetFillColor("red"s);
	polyline.SetStrokeColor("black"s);
	return polyline;
}

namespace svg {

std::ostream& operator<<(std::ostream& out, const svg::Color& roots) {
	std::visit([&out](auto value) {svg::PrintColor(out, value);}, roots);
	return out;
}

void PrintColor(std::ostream& out, std::monostate) {
	out << "none"sv;
}

void PrintColor(std::ostream& out, std::string str) {
	out << str;
}

void PrintColor(std::ostream& out, svg::Rgb& rgb) {
	out << "rgb("sv << static_cast<int>(rgb.red) << ","sv << static_cast<int>(rgb.green) << ","sv << static_cast<int>(rgb.blue) << ")"sv;
}

void PrintColor(std::ostream& out, svg::Rgba rgba) {
	out << "rgba("sv << static_cast<int>(rgba.red) << ","sv << static_cast<int>(rgba.green) << ","sv << static_cast<int>(rgba.blue) << ","sv << rgba.opacity << ")"sv;
}

std::ostream& operator<<(std::ostream& out, const svg::StrokeLineCap &enum_class) {
	switch (enum_class) {
	case svg::StrokeLineCap::BUTT:
		out << "butt"sv;
		break;
	case svg::StrokeLineCap::ROUND:
		out << "round"sv;
		break;
	case svg::StrokeLineCap::SQUARE:
		out << "square"sv;
		break;
	}
	return out;
}

std::ostream& operator<<(std::ostream& out, const svg::StrokeLineJoin &enum_class) {
	switch (enum_class) {
	case svg::StrokeLineJoin::ARCS:
		out << "arcs"sv;
		break;
	case svg::StrokeLineJoin::BEVEL:
		out << "bevel"sv;
		break;
	case svg::StrokeLineJoin::MITER:
		out << "miter"sv;
		break;
	case svg::StrokeLineJoin::MITER_CLIP:
		out << "miter-clip"sv;
		break;
	case svg::StrokeLineJoin::ROUND:
		out << "round"sv;
		break;
	}
	return out;
}

// Object ----------------------------------------------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
	objects_.push_back(move(obj));
}

void Object::Render(const RenderContext& context) const {
	context.RenderIndent();
	RenderObject(context);
	context.out << std::endl;
}

// Document ----------------------------------------------------------

void Document::Render(std::ostream& out) const  {
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
	out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
	RenderContext ctx(out, 2, 2);
	for (auto& item_ptr : objects_) {
		item_ptr->Render(ctx);
	}
	out << "</svg>"sv;
}

// Circle ----------------------------------------------------------

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
	RenderAttrs(out);
	out << "/>"sv;
}

// Polyline ----------------------------------------------------------

Polyline& Polyline::AddPoint(Point point) {
	points_.push_back(point);
	return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
	auto& out = context.out;
	//out << "  "sv;
	out << "<polyline points=\""sv;
	bool is_first = false;
	for (auto& point : points_) {
		if (is_first) {
			out << " "sv;
		}
		is_first=true;
		out << point.x << ","sv << point.y;

	}
	out << "\""sv;
	RenderAttrs(out);
	out << "/>"sv; //
	out << ' '; //
}

// Text ----------------------------------------------------------

Text& Text::SetPosition(Point pos) {
	position_ = pos;
	return *this;
}

Text& Text::SetOffset(Point offset) {
	offset_ = offset;
	return *this;
}

Text& Text::SetFontSize(uint32_t size) {
	size_ = size;
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
	data_ = data;
	return *this;
}

void Text::RenderObject(const RenderContext& context) const  {
	auto& out = context.out;
	out << "<text"sv;
	RenderAttrs(out);
	out	<< " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv
		<< "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv
		<< " font-size=\""sv << size_ << "\"" ;
	if (!font_family_.empty()) {
		out << " font-family=\""sv << font_family_ << "\""sv;
	}
	if (!font_weight_.empty()) {
		out	<< " font-weight=\""sv << font_weight_ << "\""sv;
	}
	out	<< ">"sv;
	if (!data_.empty()) {
		for (auto& item : data_) {
			if (item == '"') {
				out << "&quot;"sv;
				continue;
			}
			if (item == '\'') {
				out << "&apos;"sv;
				continue;
			}
			if (item == '<') {
				out << "&lt;"sv;
				continue;
			}
			if (item == '>') {
				out << "&gt;"sv;
				continue;
			}
			if (item == '&') {
				out << "&amp;"sv;
				continue;
			}
			out << item;
		}
	}
	out << "</text>"sv;
	}

}// namespace svg

namespace shapes {

// Triangle ----------------------------------------------------------

void Triangle::Draw(svg::ObjectContainer& container) const {
	container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
}

// Star ----------------------------------------------------------

void Star::Draw(svg::ObjectContainer& container) const {
	container.Add(CreateStar(centr_, outer_radius_, inner_radius_, rays_count_));
}

// Snowman ----------------------------------------------------------

void Snowman::Draw(svg::ObjectContainer& container) const  {
	svg::Point second_ball_centr(centr_.x, (centr_.y + radius_ * 2));
	svg::Point sthird_ball_centr(centr_.x, (centr_.y + radius_ * 5));
	container.Add(svg::Circle().SetCenter(sthird_ball_centr).SetRadius(radius_ * 2).SetFillColor(svg::Rgb{240,240,240}).SetStrokeColor("black"s));
	container.Add(svg::Circle().SetCenter(second_ball_centr).SetRadius(radius_ * 1.5).SetFillColor(svg::Rgb{240,240,240}).SetStrokeColor("black"s));
	container.Add(svg::Circle().SetCenter(centr_).SetRadius(radius_).SetFillColor(svg::Rgb{240,240,240}).SetStrokeColor("black"s));
}

}// namespace shapes


