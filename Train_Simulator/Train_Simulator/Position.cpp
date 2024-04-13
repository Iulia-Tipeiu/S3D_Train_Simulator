#include "Position.h"

Position::Position(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

Position::Position(double value) {
	x = y = z = value;
}
