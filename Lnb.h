#pragma once
#include <vector>
#include <cstdint>

class DVBSTransponder;

struct FrequencyRange {
	uint32_t low;
	uint32_t high;
	uint32_t offset;
	int32_t rangeswitch;
	uint8_t polarizations;
};

class Lnb:public std::vector<FrequencyRange> {
public:
	enum Polarization { Off = 0, Horizontal = 0b1, Vertical = 0b10, Left = 0b100, Right = 0b1000, Any = 0b1111 };
	Lnb(std::vector<FrequencyRange> const &fr):std::vector<FrequencyRange>(fr) {}
	uint32_t frequencyOffset(DVBSTransponder const &t) const;
	static const Lnb Universal;
	static const Lnb Expressvu;
	static const Lnb Extended;
	static const Lnb Standard;
	static const Lnb L10700;
	static const Lnb L11300;
	static const Lnb Enhanced;
	static const Lnb QPH031;
	static const Lnb CBand;
	static const Lnb CBandMultipoint;
	static const Lnb DishPro;
	static const Lnb L110BS;
	static const Lnb BrasilSatStacked;
	static const Lnb BrasilSatOi;
	static const Lnb BrasilSatAmazonas3;
	static const Lnb BrasilSatAmazonas2;
	static const Lnb BrasilSatGVT;
};
