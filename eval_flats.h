//------------------------------------------------------------------------------
bool	CalculateFlats	(UINT	type,  CHAR* date);
//------------------------------------------------------------------------------
class Fields {
public:
    UINT	id;
    WORD	mnem;
    UINT	id1;
    UINT	id2;
    WORD	flat;
    WORD	pip;

    Fields () {}
};

class Datas {
public:
    UINT	device;
    UINT	prm;
    DOUBLE	value;
    WORD	pipe;

    Datas () {}
};
//------------------------------------------------------------------------------

