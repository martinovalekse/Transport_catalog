#define _USE_MATH_DEFINES
#pragma once

#include <cstdint>
#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {

struct Rgb {
	Rgb() = default;
	Rgb(unsigned int a, unsigned int b, unsigned int c) : red(a), green(b), blue(c)  {	}

	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
};

struct Rgba {
	Rgba() = default;
	Rgba(unsigned int a, unsigned int b, unsigned int c, double d) : red(a), green(b), blue(c), opacity(d)  {}

	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	double opacity = 1.0;
};

using Color = std::variant<std::monostate, svg::Rgb, svg::Rgba, std::string>;
inline const Color NoneColor{"none"};

std::ostream& operator<<(std::ostream& out, const svg::Color& roots);

//-----------------------------------------------------------------

enum class StrokeLineCap {
	BUTT,
	ROUND,
	SQUARE,
};

enum class StrokeLineJoin {
	ARCS,
	BEVEL,
	MITER,
	MITER_CLIP,
	ROUND,
};

std::ostream& operator<<(std::ostream& out, const svg::StrokeLineCap &enum_class);
std::ostream& operator<<(std::ostream& out, const svg::StrokeLineJoin &enum_class);

struct Point {
	Point() = default;
	Point(double x, double y)
		: x(x)
		, y(y) {
	}
	double x = 0;
	double y = 0;
};


struct RenderContext {
	RenderContext(std::ostream& out)
		: out(out) {
	}

	RenderContext(std::ostream& out, int indent_step, int indent = 0)
		: out(out)
		, indent_step(indent_step)
		, indent(indent) {
	}

	RenderContext Indented() const {
		return {out, indent_step, indent + indent_step};
	}

	void RenderIndent() const {
		for (int i = 0; i < indent; ++i) {
			out.put(' ');
		}
	}

	std::ostream& out;
	int indent_step = 0;
	int indent = 0;
};

class Object {
public:
	virtual ~Object() = default;
	void Render(const RenderContext& context) const;


private:
	virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
	virtual ~ObjectContainer() = default;

	template <typename Obj>
	void Add(Obj obj) {
		AddPtr(std::make_unique<Obj>(std::move(obj)));
	}

	virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

class Drawable  {
public:
	virtual ~Drawable() = default;
	virtual void Draw(ObjectContainer& ) const = 0;
};

class Document : public ObjectContainer {
public:
	virtual ~Document() = default;
	void AddPtr(std::unique_ptr<Object>&& obj) override;
	void Render(std::ostream& out) const;

private:
	std::vector<std::unique_ptr<Object>>  objects_;
};

//-----------------------------------------------------------------

void PrintColor(std::ostream& out, std::monostate sroot);
void PrintColor(std::ostream& out, std::string root);
void PrintColor(std::ostream& out, svg::Rgb& roots);
void PrintColor(std::ostream& out, svg::Rgba roots);

std::ostream& operator<<(std::ostream& out, const svg::Rgb& roots);

//-----------------------------------------------------------------

template <typename Owner>
class PathProps {
public:
	Owner& SetFillColor(Color color) {
		fill_color_ = std::move(color);
		return AsOwner();
	}

	Owner& SetStrokeColor(Color color) {
		stroke_color_ = std::move(color);
		return AsOwner();
	}

	Owner& SetStrokeLineCap(StrokeLineCap type) {
		fill_linecap_ = std::move(type);
		return AsOwner();
	}

	Owner& SetStrokeLineJoin(StrokeLineJoin type) {
		stroke_linejoin_ = std::move(type);
		return AsOwner();
	}

	Owner& SetStrokeWidth(double num) {
		stroke_width_ = num;
		return AsOwner();
	}

protected:
	~PathProps() = default;

	void RenderAttrs(std::ostream& out) const {
		using namespace std::literals;
		if (fill_color_) {
			out << " fill=\""sv << *fill_color_ << "\""sv;
		}
		if (stroke_color_) {
			out << " stroke=\""sv << *stroke_color_ << "\""sv; //
		}
		if (stroke_width_) {
			out << " stroke-width=\""sv << *stroke_width_ << "\""sv; //
		}
		if (fill_linecap_) {
			out << " stroke-linecap=\""sv << *fill_linecap_ << "\""sv; //
		}
		if (stroke_linejoin_) {
			out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv; //
		}
	}

private:
	Owner& AsOwner() {
		return static_cast<Owner&>(*this);
	}

	std::optional<Color> fill_color_;
	std::optional<Color> stroke_color_;
	std::optional<StrokeLineCap> fill_linecap_;
	std::optional<StrokeLineJoin> stroke_linejoin_;
	std::optional<double> stroke_width_;
};

//-----------------------------------------------------------------

class Circle final : public Object, public PathProps<Circle> {
public:
	Circle& SetCenter(Point center);
	Circle& SetRadius(double radius);

private:
	void RenderObject(const RenderContext& context) const override;
	Point center_;
	double radius_ = 1.0;
};

class Polyline : public Object, public PathProps<Polyline> {
public:
	Polyline& AddPoint(Point point);

private:
	void RenderObject(const RenderContext& context) const override;

	std::vector<Point> points_;
};

class Text : public Object, public PathProps<Text> {
public:
	Text& SetPosition(Point pos);
	Text& SetOffset(Point offset);
	Text& SetFontSize(uint32_t size);
	Text& SetFontFamily(std::string font_family);
	Text& SetFontWeight(std::string font_weight);
	Text& SetData(std::string data);

private:
	void RenderObject(const RenderContext& context) const override;

	Point position_ = {0.0, 0.0};
	Point offset_ = {0.0, 0.0};
	uint32_t size_ = 1;
	std::string font_family_;
	std::string font_weight_;
	std::string data_;
};

}// namespace svg

svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays); // in global namespace

namespace shapes {

class Triangle : public svg::Drawable {
public:
	Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
		: p1_(p1)
		, p2_(p2)
		, p3_(p3) {
	}

	void Draw(svg::ObjectContainer& container) const override;

private:
	svg::Point p1_, p2_, p3_;
};

class Star : public svg::Drawable{
public:
	Star(svg::Point centr, double outer_radius, double innder_radius, int rays_count)
		: centr_(centr)
		, outer_radius_(outer_radius)
		, inner_radius_(innder_radius)
		, rays_count_(rays_count) {
	}

	void Draw(svg::ObjectContainer& container) const override;

private:
	 svg::Point centr_;
	 double outer_radius_;
	 double inner_radius_;
	 int rays_count_;
};

class Snowman : public svg::Drawable {
public:
	Snowman(svg::Point centr, double radius)
		: centr_(centr)
		, radius_(radius) {
	}

	void Draw(svg::ObjectContainer& container) const override;

private:
	svg::Point centr_;
	double radius_;
};

}// namespace shapes
