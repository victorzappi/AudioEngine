/*
 * Quad.h
 *
 *  Created on: 2016-01-21
 *      Author: vic
 */

#ifndef QUAD_H_
#define QUAD_H_

class Quad {
public:
	Quad();
	virtual ~Quad();

	virtual void init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy);
	float getAttribute(int index);
	void copy(Quad *q);
	virtual void setQuad(int *params) = 0;
protected:
	void allocateStructures();
	void deallocateStructures();

	int numOfStates;

	int numOfAttributes;

	int textureW;
	int textureH;

	float deltaTexX;
	float deltaTexY;

	float *attributes;
};

inline float Quad::getAttribute(int index) {
	return attributes[index];
}



//----------------------------------------------------------------------------------------------------------------------------------
class QuadExplicitFDTD : public Quad {
public:
	QuadExplicitFDTD();
	~QuadExplicitFDTD();

	void init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy); // stateNum and  attrNum are passed as a check

	void setQuad(int *params);
	virtual void setQuad(int topleftX, int topleftY, int width, int height, int coordNShiftX);
	virtual void setQuadAudio(int topleftX, int topleftY, int width, int height);
	virtual void setQuadDrawer(int topleftX, int topleftY, int width, int height);

protected:
	virtual void setVertexPositions(int topleftX, int topleftY, int width, int height);

	// 4 vertices, each with these numOfAttributes attributes

	// 2 for vertex pos (x, y) -> used only in vertex shader
	float vertx[4];
	float verty[4];
	// 2 for texture pos (x, y) of this cell at current state
	float texCoord0x[4];
	float texCoord0y[4];

	// 2 for texture pos (x, y) of left neighbor cell at current state
	float texCoord1x[4];
	float texCoord1y[4];
	// 2 for texture pos (x, y) of up neighbor cell at current state
	float texCoord2x[4];
	float texCoord2y[4];

	// 2 for texture pos (x, y) of right neighbor cell at current state
	float texCoord3x[4];
	float texCoord3y[4];
	// 2 for texture pos (x, y) of down neighbor cell at current state
	float texCoord4x[4];
	float texCoord4y[4];
};



//----------------------------------------------------------------------------------------------------------------------------------
class QuadImplicitFDTD : public Quad {
public:
	QuadImplicitFDTD();
	~QuadImplicitFDTD();

	void init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy); // stateNum and  attrNum are passed as a check

	void setQuad(int *params);
	virtual void setQuad(int topleftX, int topleftY, int width, int height,
		    int coordNShiftX, int coordNShiftY, int coordN_1ShiftX, int coordN_1ShiftY);
	virtual void setQuadAudio(int topleftX, int topleftY, int width, int height);
	virtual void setQuadExcitation(int topleftX, int topleftY, int width, int height,
		    int prevShiftX, int prevShiftY, int folShiftX, int folShiftY, int quadIndex);
	virtual void setQuadDrawer(int topleftX, int topleftY, int width, int height);

protected:
	virtual void setVertexPositions(int topleftX, int topleftY, int width, int height);
	virtual void nextExcitation(float prevShiftFragX, float prevShiftFragY, int &nextIndex, float &nextCoordX, float &nextCoordY);

	// 4 vertices, each with these numOfAttributes attributes

	// 2 for vertex pos (x, y)
	float vertx[4];
	float verty[4];
	// 2 for texture pos (x, y) of this cell at current state
	float texCoord0x[4];
	float texCoord0y[4];

	// 2 for texture pos (x, y) of left neighbor cell at current state
	float texCoord1x[4];
	float texCoord1y[4];
	// 2 for texture pos (x, y) of up neighbor cell at current state
	float texCoord2x[4];
	float texCoord2y[4];

	// 2 for texture pos (x, y) of right neighbor cell at current state
	float texCoord3x[4];
	float texCoord3y[4];
	// 2 for texture pos (x, y) of down neighbor cell at current state
	float texCoord4x[4];
	float texCoord4y[4];

	// 2 for texture pos (x, y) of left/up neighbor cell at current state
	float texCoord5x[4];
	float texCoord5y[4];
	// 2 for texture pos (x, y) of right/down neighbor cell at current state
	float texCoord6x[4];
	float texCoord6y[4];

	// 2 for texture pos (x, y) of this cell at previous state
	float texCoord7x[4];
	float texCoord7y[4];
	// 2 for texture pos (x, y) of left neighbor cell at previous state
	float texCoord8x[4];
	float texCoord8y[4];

	// 2 for texture pos (x, y) of up neighbor cell at previous state
	float texCoord9x[4];
	float texCoord9y[4];
};

#endif /* QUAD_H_ */
