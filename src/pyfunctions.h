#include <Python.h>
#include <stdio.h>
#include <conio.h>

char * myFileString;
char * myCourseString;
char * mySpeedString;
char * myLatString;
char * myLonString;

static int numargs = 0;

/*
*
*
*Return the number of arguments of the application command line
*
*
*/
static PyObject*
emb_numargs(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":numargs"))
		return NULL;
	return PyLong_FromLong(numargs);
}


static PyMethodDef EmbMethods[] = {
	{ "numargs", emb_numargs, METH_VARARGS,
	"Return the number of arguments received by the process." },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef EmbModule = {
	PyModuleDef_HEAD_INIT, "emb", NULL, -1, EmbMethods,
	NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_emb(void)
{
	return PyModule_Create(&EmbModule);
}


/*
//
//
//
// function for passing Python script file name
//
//
//
*/
static char* myFile;

/* Return a character ... command line */
static PyObject*
file_argstring(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":myFileString"))
		return NULL;
	return PyUnicode_FromString(myFileString);
}

static PyMethodDef FileMethods[] = {
	{ "myFile", file_argstring, METH_VARARGS,
	"Returns the keywords received by the process." },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef FileModule = {
	PyModuleDef_HEAD_INIT, "filestring", NULL, -1, FileMethods,
	NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_file(void)
{
	return PyModule_Create(&FileModule);
}


/*
//
//
//
// function for passing course
//
//
//
*/
static char* myCourse;

/* Return a character ... command line */
static PyObject*
cse_argstring(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":myCourseString"))
		return NULL;
	return PyUnicode_FromString(myCourseString);
}

static PyMethodDef CseMethods[] = {
	{ "myCourse", cse_argstring, METH_VARARGS,
	"Returns the keywords received by the process." },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef CseModule = {
	PyModuleDef_HEAD_INIT, "course", NULL, -1, CseMethods,
	NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_course(void)
{
	return PyModule_Create(&CseModule);
}

/*
//
//
//
// function for passing speed
//
//
//
*/
static char* mySpeed;

/* Return a character ... command line */
static PyObject*
spd_argstring(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":mySpeedString"))
		return NULL;
	return PyUnicode_FromString(mySpeedString);
}

static PyMethodDef SpdMethods[] = {
	{ "mySpeed", spd_argstring, METH_VARARGS,
	"Returns the keywords received by the process." },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef SpdModule = {
	PyModuleDef_HEAD_INIT, "speed", NULL, -1, SpdMethods,
	NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_speed(void)
{
	return PyModule_Create(&SpdModule);
}

/*
//
//
//
// function for passing latitude
//
//
//
*/
static char* myLat;

/* Return a character ... command line */
static PyObject*
lat_argstring(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":myLatString"))
		return NULL;
	return PyUnicode_FromString(myLatString);
}

static PyMethodDef LatMethods[] = {
	{ "myLat", lat_argstring, METH_VARARGS,
	"Returns the keywords received by the process." },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef LatModule = {
	PyModuleDef_HEAD_INIT, "lat", NULL, -1, LatMethods,
	NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_lat(void)
{
	return PyModule_Create(&LatModule);
}

/*
//
//
//
// function for passing longitude
//
//
//
*/
static char* myLon;

/* Return a character ... command line */
static PyObject*
lon_argstring(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":myLonString"))
		return NULL;
	return PyUnicode_FromString(myLonString);
}

static PyMethodDef LonMethods[] = {
	{ "myLon", lon_argstring, METH_VARARGS,
	"Returns the keywords received by the process." },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef LonModule = {
	PyModuleDef_HEAD_INIT, "lon", NULL, -1, LonMethods,
	NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_lon(void)
{
	return PyModule_Create(&LonModule);
}