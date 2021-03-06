/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2011-2012   Christian Urich
 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <dmedge.h>
#include <dmnode.h>
#include <dmface.h>
#include <dmlogger.h>

#include "cgalgeometry_p.h"
#include <CGAL/Snap_rounding_2.h>

namespace DM {


Segment_list_2 CGALGeometry_P::Snap_Rounding_2D(DM::System * sys, DM::View & view, float tol)  {

	Logger(Debug) << "Snap Rounding 2D";

	Segment_list_2 seg_list;
	Polyline_list_2 output_list;

	seg_list = CGALGeometry_P::EdgeToSegment2D(sys, view);

	// Generate an iterated snap-rounding
	CGAL::snap_rounding_2<Traits,Segment_list_2::const_iterator,Polyline_list_2>
			(seg_list.begin(), seg_list.end(), output_list, tol, false, false, 1);

	Segment_list_2 ret_list;
	ret_list = CGALGeometry_P::PolyLineToSegments(output_list);

	Logger(Debug) << "End Snap Rounding 2D";

	return ret_list;
}

Segment_list_2 CGALGeometry_P::EdgeToSegment2D(DM::System * sys,  DM::View & view) {
	Segment_list_2 seg_list;

	if (view.getType() == DM::EDGE) {
		foreach(DM::Component * c, sys->getAllComponentsInView(view)) {
			DM::Edge * edge = static_cast<DM::Edge*>(c);
			DM::Node * n1 = edge->getEndNode();
			DM::Node * n2 = edge->getStartNode();
			Segment_2 seg(Point_2(n1->getX(), n1->getY()), Point_2(n2->getX(), n2->getY()));
			if (!seg.is_degenerate () ) {
				seg_list.push_back(seg);
			}
		}
		return seg_list;
	}
	int face_counter = 0;
	int hole_counter = 0;
	if (view.getType() == DM::FACE) {
		foreach(DM::Component * c, sys->getAllComponentsInView(view)) {
			face_counter++;
			DM::Face * f = static_cast<DM::Face*>(c);
			std::vector<DM::Node * > nodes = f->getNodePointers();
			if (nodes[0] != nodes[nodes.size()-1])
				nodes.push_back(nodes[0]);
			for (int i = 1; i < nodes.size(); i++) {
				DM::Node * n1 = nodes[i-1];
				DM::Node * n2 = nodes[i];
				Segment_2 seg(Point_2(n1->getX(), n1->getY()), Point_2(n2->getX(), n2->getY()));
				if (!seg.is_degenerate () ) {
					seg_list.push_back(seg);
				}
			}
			foreach (DM::Face * h, f->getHolePointers() ) {
				hole_counter++;
				std::vector<DM::Node * > nodes_h = h->getNodePointers();
				if (nodes_h[0] != nodes_h[nodes_h.size()-1])
					nodes_h.push_back(nodes_h[0]);
				for (int i = 1; i < nodes_h.size(); i++) {
					DM::Node * n1 = nodes_h[i-1];
					DM::Node * n2 = nodes_h[i];
					Segment_2 seg(Point_2(n1->getX(), n1->getY()), Point_2(n2->getX(), n2->getY()));
					if (!seg.is_degenerate () ) {
						seg_list.push_back(seg);
					}
				}
			}
		}
		Logger(Debug) << "Number of Faces" << face_counter;
		Logger(Debug) << "Number of Holes" << hole_counter;
		return seg_list;
	}

	DM::Logger(DM::Warning) << "Data type not supported by EdegeToSegment2D";
	return seg_list;

}

Segment_list_2 CGALGeometry_P::PolyLineToSegments(const Polyline_list_2 & poly_list) {
	Segment_list_2 seg_list;
	for (Polyline_list_2::const_iterator poly = poly_list.begin(); poly != poly_list.end(); ++poly) {
		Polyline_2::const_iterator point,  prevPoint;

		for (point = poly->begin(); point != poly->end(); ++point) {
			if (point != poly->begin()) {
				Segment_2 seg(Point_2(prevPoint->x(), prevPoint->y()), Point_2(point->x(), point->y()));
				seg_list.push_back(seg);
			}
			prevPoint = point;
		}
	}
	return seg_list;
}

DM::System CGALGeometry_P::Segment2DToEdge(Segment_list_2 seg_list, DM::View & view) {
	DM::System  sys;
	for ( Segment_list_2::const_iterator seg = seg_list.begin(); seg != seg_list.end(); ++seg) {
		double x1 = CGAL::to_double(seg->source().x());
		double y1 = CGAL::to_double(seg->source().y());
		double x2 = CGAL::to_double(seg->target().x());
		double y2 = CGAL::to_double(seg->target().y());
		DM::Node * n1 = sys.addNode(x1, y1, 0);
		DM::Node * n2 = sys.addNode(x2, y2, 0);
		sys.addEdge(n1, n2, view);
	}
	return sys;
}
double CGALGeometry_P::NumberTypetoFloat(Number_type n) {
	std::stringstream num;
	std::stringstream den;
	num << n.numerator();
	den << n.denominator();
	double numf =  QString::fromStdString(num.str()).toDouble();
	double denf =  QString::fromStdString(den.str()).toDouble();

	return numf/denf;
}

void CGALGeometry_P::AddFaceToArrangement(Arrangement_2 &arr, Face *f)
{
	Segment_list_2 seg_list;
	std::vector<DM::Node * > nodes = f->getNodePointers();
	if (nodes[0] != nodes[nodes.size()-1])
		nodes.push_back(nodes[0]);
	for (int i = 1; i < nodes.size(); i++) {
		DM::Node * n1 = nodes[i-1];
		DM::Node * n2 = nodes[i];
		Segment_2 seg(Point_2(n1->getX(), n1->getY()), Point_2(n2->getX(), n2->getY()));
		if (!seg.is_degenerate () ) {
			seg_list.push_back(seg);
		}
	}
	insert(arr, seg_list.begin(), seg_list.end());

	foreach (DM::Face * h, f->getHolePointers() ) {
		Segment_list_2 seg_holes;
		std::vector<DM::Node * > nodes_h = h->getNodePointers();
		if (nodes_h[0] != nodes_h[nodes_h.size()-1])
			nodes_h.push_back(nodes_h[0]);
		Arrangement_2 hole;
		for (int i = 1; i < nodes_h.size(); i++) {


			DM::Node * n1 = nodes_h[i-1];
			DM::Node * n2 = nodes_h[i];
			Segment_2 seg(Point_2(n1->getX(), n1->getY()), Point_2(n2->getX(), n2->getY()));
			if (!seg.is_degenerate () ) {
				seg_holes.push_back(seg);
			}
		}
		insert(arr, seg_holes.begin(), seg_holes.end());
	}


}

struct FaceInfo2
{
	FaceInfo2(){}
	int nesting_level;

	bool in_domain(){
		return nesting_level%2 == 1;
	}
};


bool  CGALGeometry_P::CheckPoints(const Point_2 & p1, const Point_2 & p2) {
	return p1==p2;
}
int CGALGeometry_P::CountNeighboringVertices (Arrangement_2::Vertex_const_handle v)
{
	int counter = 0;
	if (v->is_isolated()) {
		return counter;
	}

	Arrangement_2::Halfedge_around_vertex_const_circulator first, curr;
	first = curr = v->incident_halfedges();

	do {
		// Note that the current halfedge is directed from u to v:
		Arrangement_2::Vertex_const_handle u = curr->source();
		counter++;
	} while (++curr != first);
	return counter;
}


/*
	 VectorData Geometry::DrawTemperaturAnomaly(Point p, double l1, double l2, double b, double T) {

	 if (l1 < 1 || l2 < 1 || b < 1 ) {

	 std::cout << "Warning l1 or l2 or b is zero" << std::endl;
	 return VectorData();
	 }
	 double spliter = 40;
	 std::vector<Point> points;
	 std::vector<Edge> edges;
	 std::vector<Face> faces;
	 double t = l1/3.;
	 //std::cout << "Hello From Anomaly" << std::endl;
	 double l = l1+l2;
	 ap::real_1d_array x_x;
	 ap::real_1d_array x_y;

	 spline1dinterpolant x;
	 spline1dinterpolant z;
	 int n = 4;
	 x_x.setlength(n);
	 x_y.setlength(n);
	 x_x(0) = 0;
	 x_y(0) = 0;
	 x_x(1) = l/3.5;
	 x_y(1) = b/2.;
	 x_x(2) = l/3.;
	 x_y(2) = b/2.;
	 x_x(3) = l;
	 x_y(3) = 0;
	 spline1dbuildcubic(x_x, x_y, n, 0, double(500), 0, double(-500), x);

	 ap::real_1d_array z_x;
	 ap::real_1d_array z_y;
	 z_x.setlength(n);
	 z_y.setlength(n);
	 z_x(0) = 0;
	 z_y(0) = 0;
	 z_x(1) = l/3.5;
	 z_y(1) = t/2.;
	 z_x(2) = l/3.;
	 z_y(2) = t/2.;
	 z_x(3) = l;
	 z_y(3) = 0;

	 spline1dbuildcubic(z_x, z_y, n, 0, double(3), 0, double(-3), z);
	 long k = 0;
	 for (double i = l/spliter; i < l; i +=l/spliter) {
	 k++;
	 spline1dinterpolant y;

	 double l_y = 2 * spline1dcalc(x, i);
	 if (l_y < 0.001) {
	 l_y =  0.001;
	 }
	 //std::cout << l_y << std::endl;
	 double l_z = 2 * spline1dcalc(z, i);

	 ap::real_1d_array y_x;
	 ap::real_1d_array y_y;

	 int nl = 3;
	 y_x.setlength(nl);
	 y_y.setlength(nl);

	 y_x(0) = -l_y/2;
	 y_y(0) = 0;
	 y_x(1) = 0;
	 y_y(1) = l_z;
	 y_x(2) = l_y/2;
	 y_y(2) = 0;

	 spline1dbuildcubic(y_x, y_y, nl, 0, double(3), 0, double(-3), y);
	 int counter  = 0;
	 for (double j = -l_y/2; j <= l_y/2*1.0001; j +=l_y/spliter ) {

	 points.push_back(Point(i+ p.x-l2,j+p.y, -spline1dcalc(y, j)));
	 counter++;

	 }

	 if (k > 0 && k < spliter-1) {
	 for ( int j = 0; j < spliter;  j++) {
	 std::vector<long> ids;
	 long id;
	 id = (k-1)*(spliter+1)+j;
	 ids.push_back(id);

	 id = (k)*(spliter+1)+j;
	 ids.push_back(id);

	 id = (k)*(spliter+1)+j +1;
	 ids.push_back(id);

	 id = (k -1)*(spliter+1)+j +1;
	 ids.push_back(id);

	 id = (k-1)*(spliter+1)+j;
	 ids.push_back(id);

	 Face f(ids);
	 faces.push_back(f);
	 }

	 }


	 }

	 VectorData vec;
	 std::vector<Point> p_above;
	 std::vector<Point> p_above_side;
	 std::vector<Point> p_color;
	 //Horizontal Faces

	 for (double i = 1; i <= T; i++) {
	 ap::real_1d_array x_x;
	 ap::real_1d_array x_y;
	 spline1dinterpolant x;
	 int n = 5;
	 x_x.setlength(n);
	 x_y.setlength(n);
	 x_x(0) = 0;
	 x_y(0) = 0;
	 x_x(1) = (i/T)*l/50;
	 x_y(1) = (i/T)*b/4.;
	 x_x(2) = (i/T)*l/15.;
	 x_y(2) = (i/T)*b/2.4;
	 x_x(3) = (i/T)*l/3.5;
	 x_y(3) = (i/T)*b/2.;
	 x_x(4) = (i/T)*l;
	 x_y(4) = 0;

	 double R = 0 + 100. /(T)*(float) i;
	 double G = 0.+ 149. /(T)*(float) i;
	 double B = 139.+ 98. /(T)*(float) i;
	 spline1dbuildcubic(x_x, x_y, n, 0, double(1), 0, double(0), x);
	 for (double j =0; j < (i/T)*l+0.00001; j +=(i/T)*l/spliter) {
	 double l_y = spline1dcalc(x, j);
	 p_above.push_back(Point(j+p.x-(i/T)*l2, l_y+p.y, 0));
	 p_above_side.push_back(Point(j+p.x-(i/T)*l2, p.y-l_y, 0));

	 p_color.push_back(Point(R/255.,G/255.,B/255.));

	 }

	 }

	 p_above.push_back(Point(p.x, p.y, 0));
	 p_above_side.push_back(Point(p.x, p.y, 0));
	 p_color.push_back(Point(0./255.,0./255.,139./255.));
	 spliter++;
	 std::vector<Face> f_above;
	 std::vector<Face> f_above_side;
	 for (double i = 0; i < T-1; i++) {
	 for ( int j = 0; j < spliter-1;  j++) {
	 Face f;
	 f.ids.push_back(j+(i+1)*spliter);
	 f.ids.push_back(j+(i+1)*spliter+1);
	 f.ids.push_back(j+i*spliter+1);
	 f.ids.push_back(j+i*spliter);
	 f_above.push_back(f);
	 f_above_side.push_back(f);
	 }
	 }

	 for ( int j = 0; j < spliter-1;  j++) {
	 int i = 0;
	 Face f;
	 f.ids.push_back(j+i*spliter+1);
	 f.ids.push_back(j+i*spliter);
	 f.ids.push_back( p_above.size()-1);
	 f_above.push_back(f);
	 f_above_side.push_back(f);
	 }


	 vec.setPoints("COLOR_WF_above", p_color);
	 vec.setPoints("WF_above", p_above);
	 vec.setPoints("COLOR_WF_above_side", p_color);
	 vec.setPoints("WF_above_side",p_above_side);
	 vec.setFaces("WF_above", f_above);
	 vec.setFaces("WF_above_side", f_above_side);
	 vec.setPoints("WF", points);
	 vec.setFaces("WF", faces);
	 std::cout << "... Done " << std::endl;
	 return vec;

	 }


	 */

/*VectorData Geometry::createRaster(std::vector<Point> & points, double width, double height) {
	 //typedef CGAL::Quotient<CGAL::Gmpq>           FT;
	 //typedef long double                           FT;
	 typedef CGAL::Cartesian<CGAL::Gmpq>                   K;
	 typedef K::Point_2                            Point_2;
	 typedef CGAL::Aff_transformation_2<K>         Transformation;
	 typedef CGAL::Polygon_2<K>                    Polygon_2;
	 typedef CGAL::Polygon_with_holes_2<K>         Polygon_with_holes_2;
	 typedef std::list<Polygon_with_holes_2>       Pwh_list_2;
	 typedef CGAL::Polygon_set_2<K>                Polygon_set_2;

	 double pi =  3.14159265358979323846;
	 double area = -1;
	 double angel = 0;
	 double x;
	 double y;

	 int RasterCounter = 0;
	 VectorData vec;
	 Point transP;
	 Polygon_2 poly_s;
	 Polygon_2 poly_i;
	 poly_i.push_back(Point_2(0,0));
	 poly_i.push_back(Point_2(0,4000));
	 poly_i.push_back(Point_2(8000,4000));
	 poly_i.push_back(Point_2(8000,0));
	 CGAL::Orientation  orient = poly_i.orientation();
	 if (orient == CGAL::COUNTERCLOCKWISE) {
	 poly_i.reverse_orientation();
	 }

	 bool pointInside = false;
	 BOOST_FOREACH(Point p, points) {
	 if (p.x > 0 && p.y > 0 && p.y < 4000 && p.x < 8000) {
	 pointInside = true;
	 }
	 poly_s.push_back(Point_2(p.x, p.y));
	 }
	 if (pointInside == false){
	 std::vector<double> da;
	 da.push_back(0);
	 vec.setDoubleAttributes("numberOfRaster", da);
	 return vec;
	 }
	 std::vector<Point> poly_new = points;

	 //if ((CGAL::do_intersect(poly_i, poly_s))) {
	 poly_new.clear();
	 points.clear();
	 //std::cout << "START INTERSECT" << std::endl;
	 Pwh_list_2                  intR;
	 Pwh_list_2::const_iterator  it;
	 CGAL::difference (poly_s,poly_i , std::back_inserter(intR));
	 for (it = intR.begin(); it != intR.end(); ++it) {
	 int counter = 0;
	 Polygon_with_holes_2 ph = *(it);
	 //int dh = pl_new.size();
	 for ( int i = 0; i < ph.outer_boundary().size(); i++) {
	 Point_2 p =   ph.outer_boundary()[i];
	 poly_new.push_back(Point(CGAL::to_double(p.x()), CGAL::to_double(p.y()), 0));
	 counter++;
	 }
	 }
	 points = poly_new;
	 //std::cout << "<-------------END INTERSECT" << std::endl;
	 //}
	 //std::cout << poly_new.size() << std::endl;
	 poly_s.clear();


	 //std::cout << "Begin intersect" << std::endl;
	 points = Geometry::offsetPolygon(points);
	 //Intersect


	 BOOST_FOREACH(Point p, points) {
	 poly_s.push_back(Point_2(p.x, p.y));
	 }
	 if (points.size() < 2)  {
	 //std::cout << "Error: offsetPolygon empty" << std::endl;
	 std::vector<double> da;
	 da.push_back(0);
	 vec.setDoubleAttributes("numberOfRaster", da);
	 return vec;
	 } else {
	 if(!poly_s.is_simple()) {
	 //std::cout << "ERRRRRRRRRRRORRRRRRRR NOT SIMPLE" << std::endl;
	 return vec;
	 }
	 CGAL::Orientation  orient = poly_s.orientation();
	 if (orient == CGAL::COUNTERCLOCKWISE) {
	 poly_s.reverse_orientation();
	 }
	 //std::cout << "Find min Rectangle huhuh" << std::endl;
	 for (double i = 0; i < 180; i++) {
	 Transformation rotate(CGAL::ROTATION, sin(i/180.*pi), cos(i/180.*pi));
	 std::list<Point_2> points_2;
	 //std::cout << "Find min Rectangle 1" << std::endl;
	 //std::cout << points.size() <<std::endl;
	 BOOST_FOREACH(Point p, points) {
	 //std::cout << p.x << "/" << p.y << std::endl;
	 points_2.push_back(rotate(Point_2(p.x, p.y)));
	 }
	 //std::cout << "Find min Rectangle 2" << std::endl;
	 K::Iso_rectangle_2 c2 = CGAL::bounding_box(points_2.begin(), points_2.end());
	 double a=CGAL::to_double( c2.area());
	 //std::cout << "Find min Rectangle 3" << std::endl;
	 if (area < 0 || a < area ) {
	 area = a;
	 angel = i;
	 x = CGAL::to_double(c2.xmax()-c2.xmin());
	 y = CGAL::to_double(c2.ymax()-c2.ymin());
	 transP = Point(CGAL::to_double(c2.xmin()), CGAL::to_double(c2.ymin()), 0);

	 }
	 }
	 //std::cout << "end Find min Rectangle new" << std::endl;

	 double l = x;
	 double b = y;

	 //std::cout << l << "/" << b << std::endl;
	 int dl = (int) l/height ;
	 int db = (int) b/width ;
	 //std::cout << dl << "/" << db  << std::endl;
	 std::vector<Point> pl;
	 std::list<Point_2> points__list_2;
	 if (angel > 180 ) {
	 angel-=180;
	 }
	 if(l <= 0 || b <= 0 || dl <= 0 || db <= 0) {
	 return vec;
	 //std::cout <<"ERRRRRRRRRRROR"<<std::endl;
	 }
	 Transformation rotate_back(CGAL::ROTATION, sin(-angel/180.*pi), cos(-angel/180.*pi));
	 for ( int i = 0; i <= dl; i++ ) {
	 for ( int j = 0; j <= db; j++) {
	 //std::cout << "sd1" << pl.size() << std::endl;
	 Point_2 p =rotate_back(Point_2((double)i*l/(double) (dl) + transP.x,  (double) j * b/(double) (db)+transP.y));
	 //std::cout << p << std::endl;
	 //std::cout << "sd2" << pl.size() << std::endl;
	 pl.push_back(Point(CGAL::to_double(p.x()), CGAL::to_double(p.y()), 0 ));

	 }
	 }
	 //std::cout << "rrrrr" << pl.size()<< std::endl;
	 for ( int i = 0; i < dl; i++) {
	 for ( int j = 0; j < db; j++){
	 //std::cout << "inters0" << pl.size()<< " " << i<< "/" << j << std::endl;
	 Polygon_2 poly;


	 int id1 = (db+1)*i+j;
	 int id2 = (db+1)*(i+1)+j;
	 int id3 = (db+1)*(i+1)+j+1 ;
	 int id4 = (db+1)*i+j+1;


	 poly.push_back(Point_2(pl[id1].x, pl[id1].y));
	 //std::cout << "0" << std::endl;
	 poly.push_back(Point_2(pl[id2].x, pl[id2].y));
	 //std::cout << "1" << std::endl;
	 poly.push_back(Point_2(pl[id3].x, pl[id3].y));
	 //std::cout << "2" << std::endl;
	 poly.push_back(Point_2(pl[id4].x, pl[id4].y));
	 //std::cout << "3" << std::endl;


	 CGAL::Orientation  orient = poly.orientation();
	 if (orient == CGAL::CLOCKWISE) {
	 poly.reverse_orientation();
	 }
	 ////std::cout << poly_s.size() << std::endl;
	 //std::cout << poly<< std::endl;
	 //std::cout << poly_s << std::endl;
	 if ((CGAL::do_intersect(poly_s, poly))) {
	 //std::cout << "inters1" << std::endl;
	 Pwh_list_2                  intR;
	 Pwh_list_2::const_iterator  it;
	 CGAL::difference (poly,poly_s , std::back_inserter(intR));
	 for (it = intR.begin(); it != intR.end(); ++it) {
	 std::vector<Face> faces;
	 std::vector<Edge> edges;
	 std::vector<Point> pl_new;

	 int counter = 0;
	 Polygon_with_holes_2 ph = *(it);
	 int dh = pl_new.size();
	 for ( int i = 0; i < ph.outer_boundary().size(); i++) {
	 Point_2 p =   ph.outer_boundary()[i];
	 pl_new.push_back(Point(CGAL::to_double(p.x()), CGAL::to_double(p.y()), 0));
	 counter++;
	 }
	 //std::cout << counter  <<std::endl;
	 if (counter > 2) {
	 Face f;
	 for (int i = 0; i < counter; i++) {
	 f.ids.push_back(dh+1);
	 }
	 faces.push_back(f);
	 for (int i = 1; i < counter; i++) {
	 edges.push_back(Edge(dh+i-1, dh+i));
	 }
	 edges.push_back(Edge(dh+counter-1, dh));
	 std::stringstream s;
	 s << "Raster" << RasterCounter;
	 vec.setPoints(s.str() , pl_new);
	 vec.setEdges(s.str() , edges);
	 vec.setFaces(s.str() , faces);
	 RasterCounter++;
	 }
	 }

	 } else {
	 //std::cout << "inters2" << std::endl;
	 std::vector<Face> faces;
	 std::vector<Edge> edges;
	 std::vector<Point> pl_new;
	 int dh = pl_new.size();
	 Point_2 p;
	 // p.

	 int counter = 0;
	 bool outside = false;
	 for ( int i = 0; i < poly.size(); i++) {

	 Point_2 p =  poly[i];

	 if (CGAL::ON_UNBOUNDED_SIDE == CGAL::bounded_side_2(poly_s.vertices_begin(), poly_s.vertices_end(),p , K())) {
	 outside =true;
	 }

	 pl_new.push_back(Point(CGAL::to_double(p.x()), CGAL::to_double(p.y()), 0));
	 counter++;
	 if(outside) {
	 counter = 0;
	 }
	 }
	 //std::cout << "done" << counter << std::endl;
	 if (counter > 2) {
	 Face f;
	 for (int i = 0; i < counter; i++) {
	 f.ids.push_back(dh+1);
	 }
	 faces.push_back(f);
	 for (int i = 1; i < counter; i++) {
	 edges.push_back(Edge(dh+i-1, dh+i));
	 }
	 edges.push_back(Edge(dh+counter-1, dh));
	 std::stringstream s;
	 s << "Raster" << RasterCounter;
	 vec.setPoints(s.str() , pl_new);
	 vec.setEdges(s.str() , edges);
	 vec.setFaces(s.str() , faces);
	 RasterCounter++;
	 }
	 }
	 }

	 }

	 }
	 std::vector<double> da;
	 da.push_back(RasterCounter);
	 vec.setDoubleAttributes("numberOfRaster", da);
	 //std::cout << "Finished createRasterData" << std::endl;
	 return vec;

	 }*/
}
