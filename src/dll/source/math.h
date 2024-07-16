#pragma once

#define M_PI       3.14159265358979323846 
#define DEG2RAD(nDeg) (nDeg) * M_PI / 180.f
#define RAD2DEG(nRad) (nRad) * 180.0f / M_PI

/*
	misc funcs
*/
inline void SinCos(const float& r, float& s, float& c) {
	s = sin(r);
	c = cos(r);
}

/*
	vec
*/
class Vector {
public:
	float x, y, z;

	inline float Magnitude2D() const { return sqrt(x * x + y * y); }

	inline float operator[](int i) const { return ((float*)this)[i]; };
	inline float& operator[](int i) { return ((float*)this)[i]; };

	inline float Dot(const Vector& vOther) const {
		const Vector& a = *this;
		return(a.x * vOther.x + a.y * vOther.y + a.z * vOther.z);
	}

	inline float Dot(const float* fOther) const {
		const Vector& a = *this;
		return(a.x * fOther[0] + a.y * fOther[1] + a.z * fOther[2]);
	}

	inline Vector operator+(const Vector& v) { return Vector(x += v.x, y += v.y, z += v.z); };
	inline Vector operator*(float fl) const {
		Vector vRes;
		vRes.x = x * fl;
		vRes.y = y * fl;
		vRes.z = z * fl;
		return vRes;
	};

	inline Vector operator-(const Vector& v) const {
		Vector vRes;
		vRes.x = x - v.x;
		vRes.y = y - v.y;
		vRes.z = z - v.z;
		return vRes;
	}

	inline  Vector& operator*=(float fl) {
		x *= fl;
		y *= fl;
		z *= fl;
		return *this;
	}

	inline void Rotate2D(const float& f)
	{
		float _x, _y;

		float s, c;

		SinCos(DEG2RAD(f), s, c);

		_x = x;
		_y = y;

		x = (_x * c) - (_y * s);
		y = (_x * s) + (_y * c);
	}

	inline float Length2D(void) const { return sqrtf(x * x + y * y); }
	inline float Length(void) const { return sqrtf((x * x) + (y * y) + (z * z)); }
};

/*
	vecaligned
*/
inline class __declspec(align(16)) VectorAligned : public Vector
{
public:
	inline VectorAligned(void) {};
	//inline VectorAligned(vec_t X, vec_t Y, vec_t Z)
	//{
	//	Init(X, Y, Z);
	//}

#ifdef VECTOR_NO_SLOW_OPERATIONS

private:
	// No copy constructors allowed if we're in optimal mode
	VectorAligned(const VectorAligned& vOther);
	VectorAligned(const Vector& vOther);

#else
public:
	//explicit VectorAligned(const Vector& vOther)
	//{
	//	Init(vOther.x, vOther.y, vOther.z);
	//}

	//VectorAligned& operator=(const Vector& vOther)
	//{
	//	Init(vOther.x, vOther.y, vOther.z);
	//	return *this;
	//}

#endif
	float w;	// this space is used anyway
} ALIGN16_POST;

/*
	qang
*/
class QAngle {
	public:
		float x, y, z;

		inline QAngle operator+=(const QAngle& v) {
			x += v.x; y += v.y; z += v.z;
			return *this;
		}

		inline QAngle operator-(const QAngle& v) const
		{
			QAngle vRes;
			vRes.x = x - v.x;
			vRes.y = y - v.y;
			vRes.z = z - v.z;
			return vRes;
		}

		inline float Length2D(void) const { return sqrtf((x * x) + (y * y) + (z * z)); }
		inline Vector GetForwardDirection() {
			float fSP, fSY, fCP, fCY;

			SinCos(DEG2RAD(y), fSY, fCY);
			SinCos(DEG2RAD(x), fSP, fCP);

			return Vector(fCP * fCY, fCP * fSY, -fSP);

		}
};

/*
	mat3x4
*/
class matrix3x4_t {
	public:
		float* operator[](int i) { return m_fMatVal[i]; }
		const float* operator[](int i) const { return m_fMatVal[i]; }

		float m_fMatVal[3][4];
};

// quat
class Quaternion {
	public:
		float x, y, z, w;
};

/*
	radeuler
*/
class RadianEuler {
	public:
		float x, y, z;
};

/*
	v4d
*/
class Vector4D {
	public:
		float x, y, z, w;

		inline float* Base() { return (float*)this; }
		inline const float* Base() const { return (const float*)this; }
		inline float operator[](int i) const { return ((float*)this)[i]; }
		inline float& operator[](int i) { return ((float*)this)[i]; }
};


/*
	vmat
*/
class VMatrix {
public:
	float		m[4][4];
};

/*
	misc functions
*/
inline float DotProduct(const float* v1, const float* v2) { return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]; }

inline void VectorTransform(const float* In, const matrix3x4_t& mIn, float* Out) {
	Out[0] = DotProduct(In, mIn[0]) + mIn[0][3];
	Out[1] = DotProduct(In, mIn[1]) + mIn[1][3];
	Out[2] = DotProduct(In, mIn[2]) + mIn[2][3];
}

inline void VectorTransform(const Vector& vIn, const matrix3x4_t& mIn, Vector& vOut) { VectorTransform(&vIn.x, mIn, &vOut.x); }

inline void VectorAngles(Vector& vFwd, QAngle& vAng) {
	if (vFwd.y == 0 && vFwd.x == 0) {
		vAng.x = vFwd.z > 0.f ? 270.f : 90.f;
		vAng.y = 0;
	}
	else {
		vAng.x = RAD2DEG(atan2(-vFwd.z, vFwd.Length2D()));
		if (vAng.x < 0)
			vAng.x += 360;

		vAng.y = RAD2DEG(atan2(vFwd.y, vFwd.x));
		if (vAng.y < 0)
			vAng.y += 360;
	}

	vAng.z = 0;
}

inline float Reverse(float fCoord) {
	float fRev = fCoord / 360.f;
	if (fCoord > 180.f || fCoord < -180.f) {
		fRev = abs(fRev);
		fRev = round(fRev);

		if (fCoord < 0.f)
			return fCoord + 360.f * fRev;
		else
			return fCoord - 360.f * fRev;
	}
	return fCoord;
}

inline void NormalizeAngle(QAngle& vAng) {
	vAng.x = Reverse(vAng.x);
	vAng.y = Reverse(vAng.y);
	vAng.z = Reverse(vAng.z);
}

inline QAngle CalcAngle(Vector& vSrc, Vector& vDest) {
	QAngle vAngles;
	Vector vDelta = vDest - vSrc;

	VectorAngles(vDelta, vAngles);
	NormalizeAngle(vAngles);

	return vAngles;
}

inline void VectorSubtract(const Vector& a, const Vector& b, Vector& c) {
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
}

inline void VectorClear(Vector& a) { a.x = a.y = a.z = 0.0f; }

inline void VectorCopy(const Vector& src, Vector& dst) {
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

inline void VectorAdd(const Vector& a, const Vector& b, Vector& c) {
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
}

inline QAngle VectorToAngle(const Vector& vVec) {
	return QAngle(	RAD2DEG(atan2(-vVec.z, hypot(vVec.x, vVec.y))),
					RAD2DEG(atan2(vVec.y, vVec.x)),
					0.f);
}