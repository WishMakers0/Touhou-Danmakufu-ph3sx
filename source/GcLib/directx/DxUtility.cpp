#include "source/GcLib/pch.h"

#include "DxUtility.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//ColorAccess
**********************************************************/
D3DCOLORVALUE ColorAccess::SetColor(D3DCOLORVALUE value, D3DCOLOR color) {
	float a = (float)GetColorA(color) / 255.0f;
	float r = (float)GetColorR(color) / 255.0f;
	float g = (float)GetColorG(color) / 255.0f;
	float b = (float)GetColorB(color) / 255.0f;
	value.r *= r; value.g *= g; value.b *= b; value.a *= a;
	return value;
}
D3DMATERIAL9 ColorAccess::SetColor(D3DMATERIAL9 mat, D3DCOLOR color) {
	float a = (float)GetColorA(color) / 255.0f;
	float r = (float)GetColorR(color) / 255.0f;
	float g = (float)GetColorG(color) / 255.0f;
	float b = (float)GetColorB(color) / 255.0f;
	mat.Diffuse.r *= r; mat.Diffuse.g *= g; mat.Diffuse.b *= b; mat.Diffuse.a *= a;
	mat.Specular.r *= r; mat.Specular.g *= g; mat.Specular.b *= b; mat.Specular.a *= a;
	mat.Ambient.r *= r; mat.Ambient.g *= g; mat.Ambient.b *= b; mat.Ambient.a *= a;
	mat.Emissive.r *= r; mat.Emissive.g *= g; mat.Emissive.b *= b; mat.Emissive.a *= a;
	return mat;
}
D3DCOLOR& ColorAccess::ApplyAlpha(D3DCOLOR& color, float alpha) {
	byte a = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alpha);
	byte r = ColorAccess::ClampColorRet(((color >> 16) & 0xff) * alpha);
	byte g = ColorAccess::ClampColorRet(((color >> 8) & 0xff) * alpha);
	byte b = ColorAccess::ClampColorRet((color & 0xff) * alpha);
	color = D3DCOLOR_ARGB(a, r, g, b);
	return color;
}
D3DCOLOR& ColorAccess::SetColorHSV(D3DCOLOR& color, int hue, int saturation, int value) {
	int i = (int)floor(hue / 60.0f) % 6;
	float f = (float)(hue / 60.0f) - (float)floor(hue / 60.0f);

	float s = saturation / 255.0f;

	int p = (int)gstd::Math::Round(value * (1.0f - s));
	int q = (int)gstd::Math::Round(value * (1.0f - s * f));
	int t = (int)gstd::Math::Round(value * (1.0f - s * (1.0f - f)));

	int red = 0;
	int green = 0;
	int blue = 0;
	switch (i) {
	case 0: red = value;	green = t;		blue = p; break;
	case 1: red = q;		green = value;	blue = p; break;
	case 2: red = p;		green = value;	blue = t; break;
	case 3: red = p;		green = q;		blue = value; break;
	case 4: red = t;		green = p;		blue = value; break;
	case 5: red = value;	green = p;		blue = q; break;
	}

	SetColorR(color, red);
	SetColorG(color, green);
	SetColorB(color, blue);
	return color;
}

/**********************************************************
//DxMath
**********************************************************/
bool DxMath::IsIntersected(D3DXVECTOR2& pos, std::vector<D3DXVECTOR2>& list) {
	if (list.size() <= 2)return false;

	bool res = true;
	for (int iPos = 0; iPos < list.size(); ++iPos) {
		int p1 = iPos;
		int p2 = iPos + 1;
		if (p2 >= list.size())p2 %= list.size();

		double cross_x = ((double)list[p2].x - (double)list[p1].x) * ((double)pos.y - (double)list[p1].y);
		double cross_y = ((double)list[p2].y - (double)list[p1].y) * ((double)pos.x - (double)list[p1].x);
		if (cross_x - cross_y < 0)res = false;
	}
	return res;
}
bool DxMath::IsIntersected(DxCircle& circle1, DxCircle& circle2) {
	double rx = circle1.GetX() - circle2.GetX();
	double ry = circle1.GetY() - circle2.GetY();
	double rr = circle1.GetR() + circle2.GetR();

	return (rx * rx + ry * ry) <= (rr * rr);
}
bool DxMath::IsIntersected(DxCircle& circle, DxWidthLine& line) {
	//先端もしくは終端が円内にあるかを調べる
	{
		double radius = circle.GetR();
		radius *= radius;

		double clx1 = circle.GetX() - line.GetX1();
		double cly1 = circle.GetY() - line.GetY1();
		double clx2 = circle.GetX() - line.GetX2();
		double cly2 = circle.GetY() - line.GetY2();

		double dist1 = clx1 * clx1 + cly1 * cly1;
		double dist2 = clx2 * clx2 + cly2 * cly2;

		if ((radius >= dist1 * dist1) || (radius >= dist2 * dist2))
			return true;
	}

	//線分内に円があるかを調べる
	{
		double lx1 = line.GetX2() - line.GetX1();
		double ly1 = line.GetY2() - line.GetY1();
		double cx1 = circle.GetX() - line.GetX1();
		double cy1 = circle.GetY() - line.GetY1();
		double inner1 = lx1 * cx1 + ly1 * cy1;

		double lx2 = line.GetX1() - line.GetX2();
		double ly2 = line.GetY1() - line.GetY2();
		double cx2 = circle.GetX() - line.GetX2();
		double cy2 = circle.GetY() - line.GetY2();
		double inner2 = lx2 * cx2 + ly2 * cy2;

		if (inner1 < 0 || inner2 < 0)
			return false;
	}

	double ux1 = line.GetX2() - line.GetX1();
	double uy1 = line.GetY2() - line.GetY1();
	double px = circle.GetX() - line.GetX1();
	double py = circle.GetY() - line.GetY1();

	double u = 1.0 / sqrt(ux1 * ux1 + uy1 * uy1);

	double ux2 = ux1 * u;
	double uy2 = uy1 * u;

	double d = px * ux2 + py * uy2;

	double qx = d * ux2;
	double qy = d * uy2;

	double rx = px - qx;
	double ry = py - qy;

	double e = rx * rx + ry * ry;
	double r = line.GetWidth() + circle.GetR();

	return (e < (r * r));
}
bool DxMath::IsIntersected(DxWidthLine& line1, DxWidthLine& line2) {

	return false;
}
bool DxMath::IsIntersected(DxLine3D& line, std::vector<DxTriangle>& triangles, std::vector<D3DXVECTOR3>& out) {
	out.clear();

	for (int iTri = 0; iTri < triangles.size(); ++iTri) {
		DxTriangle& tri = triangles[iTri];
		D3DXPLANE plane;//3角形の面
		D3DXPlaneFromPoints(&plane, &tri.GetPosition(0), &tri.GetPosition(1), &tri.GetPosition(2));

		D3DXVECTOR3 vOut;// 面と視線の交点の座標
		if (D3DXPlaneIntersectLine(&vOut, &plane, &line.GetPosition(0), &line.GetPosition(1))) {
			// 内外判定
			D3DXVECTOR3 vN[3];
			D3DXVECTOR3 vv1, vv2, vv3;
			vv1 = tri.GetPosition(0) - vOut;
			vv2 = tri.GetPosition(1) - vOut;
			vv3 = tri.GetPosition(2) - vOut;
			D3DXVec3Cross(&vN[0], &vv1, &vv3);
			D3DXVec3Cross(&vN[1], &vv2, &vv1);
			D3DXVec3Cross(&vN[2], &vv3, &vv2);
			if (D3DXVec3Dot(&vN[0], &vN[1]) < 0 || D3DXVec3Dot(&vN[0], &vN[2]) < 0)
				continue;
			else {// 内側(3角形に接触)
				out.push_back(vOut);
			}
		}
	}//for(int i=0;i<tris.size();i++)

	bool res = out.size() > 0;
	return res;
}

D3DXVECTOR4 DxMath::RotatePosFromXYZFactor(D3DXVECTOR4& vec, D3DXVECTOR2* angX, D3DXVECTOR2* angY, D3DXVECTOR2* angZ) {
	float vx = vec.x;
	float vy = vec.y;
	float vz = vec.z;
	
	if (angZ != nullptr) {
		float cz = angZ->x;
		float sz = angZ->y;

		vec.x = vx * cz - vy * sz;
		vec.y = vx * sz + vy * cz;
		vx = vec.x;
		vy = vec.y;
	}
	if (angX != nullptr) {
		float cx = angX->x;
		float sx = angX->y;

		vec.y = vy * cx - vz * sx;
		vec.z = vy * sx + vz * cx;
		vy = vec.y;
		vz = vec.z;
	}
	if (angY != nullptr) {
		float cy = angY->x;
		float sy = angY->y;

		vec.x = vz * sy + vx * cy;
		vec.z = vz * cy - vx * sy;
	}

	return vec;
}
