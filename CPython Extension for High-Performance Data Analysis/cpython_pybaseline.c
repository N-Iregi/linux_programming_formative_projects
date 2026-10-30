#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <float.h>

/**
 * compute_stats - calculates statistical metrics for a dataset
 * @data: array of double values
 * @n: size of the data array
 * @low: lower threshold for outlier detection
 * @high: upper threshold for outlier detection
 * @res: output array storing [mean, variance, min, max, outliers_count]
 */
static void compute_stats(double *data, Py_ssize_t n, double low,
			  double high, double *res)
{
	Py_ssize_t i, outliers = 0;
	double min_v, max_v, sum = 0.0, mean_v, var_sum = 0.0, var_v;

	min_v = data[0];
	max_v = data[0];
	for (i = 0; i < n; i++) {
		sum += data[i];
		if (data[i] < min_v)
			min_v = data[i];
		if (data[i] > max_v)
			max_v = data[i];
		if (data[i] < low || data[i] > high)
			outliers++;
	}
	mean_v = sum / (double)n;

	for (i = 0; i < n; i++) {
		var_sum += (data[i] - mean_v) * (data[i] - mean_v);
	}
	var_v = var_sum / (double)n;

	res[0] = mean_v;
	res[1] = var_v;
	res[2] = min_v;
	res[3] = max_v;
	res[4] = (double)outliers;
}

/**
 * build_norm_list - builds a normalized CPython list from raw doubles
 * @data: input double array
 * @n: number of elements in array
 * @min_v: dataset minimum value
 * @max_v: dataset maximum value
 *
 * Return: pointer to PyListObject, or NULL on allocation failure
 */
static PyObject *build_norm_list(double *data, Py_ssize_t n,
				 double min_v, double max_v)
{
	Py_ssize_t i;
	double range_v, norm_v;
	PyObject *list, *item;

	range_v = max_v - min_v;
	list = PyList_New(n);
	if (!list)
		return (NULL);

	for (i = 0; i < n; i++) {
		norm_v = (range_v == 0.0) ? 0.0 : (data[i] - min_v) / range_v;
		item = PyFloat_FromDouble(norm_v);
		if (!item) {
			Py_DECREF(list);
			return (NULL);
		}
		PyList_SET_ITEM(list, i, item);
	}
	return (list);
}

/**
 * fast_stats_analyze_and_normalize - CPython entry point for data analysis
 * @self: self module reference
 * @args: positional arguments passed from Python
 *
 * Return: PyDictObject containing analytics results, or NULL on error
 */
static PyObject *fast_stats_analyze_and_normalize(PyObject *self,
						  PyObject *args)
{
	PyObject *seq_obj, *fast_seq, *norm_list;
	PyObject **items;
	double threshold_low, threshold_high, stats[5], *data;
	Py_ssize_t n, i;

	(void)self;
	if (!PyArg_ParseTuple(args, "Odd", &seq_obj, &threshold_low,
			      &threshold_high))
		return (NULL);

	fast_seq = PySequence_Fast(seq_obj, "Expected a sequence of numbers");
	if (!fast_seq)
		return (NULL);

	n = PySequence_Fast_GET_SIZE(fast_seq);
	if (n == 0) {
		Py_DECREF(fast_seq);
		PyErr_SetString(PyExc_ValueError, "Dataset cannot be empty");
		return (NULL);
	}

	items = PySequence_Fast_ITEMS(fast_seq);
	data = PyMem_Malloc(n * sizeof(double));
	if (!data) {
		Py_DECREF(fast_seq);
		return (PyErr_NoMemory());
	}

	for (i = 0; i < n; i++) {
		data[i] = PyFloat_AsDouble(items[i]);
		if (PyErr_Occurred()) {
			PyMem_Free(data);
			Py_DECREF(fast_seq);
			return (NULL);
		}
	}
	Py_DECREF(fast_seq);

	compute_stats(data, n, threshold_low, threshold_high, stats);
	norm_list = build_norm_list(data, n, stats[2], stats[3]);
	PyMem_Free(data);
	if (!norm_list)
		return (NULL);

	return (Py_BuildValue("{s:d, s:d, s:d, s:d, s:n, s:N}",
			      "mean", stats[0], "variance", stats[1],
			      "min", stats[2], "max", stats[3],
			      "outliers_count", (Py_ssize_t)stats[4],
			      "normalized", norm_list));
}

/**
 * FastStatsMethods - method table mapping Python names to C functions
 */
static PyMethodDef FastStatsMethods[] = {
	{"analyze_and_normalize", fast_stats_analyze_and_normalize,
	 METH_VARARGS, "Accelerated statistical analysis in C."},
	{NULL, NULL, 0, NULL}
};

/**
 * fast_stats_module - CPython module definition structure
 */
static struct PyModuleDef c_python_pybaseline_module = {
	PyModuleDef_HEAD_INIT,
	"c_python_pybaseline",
	"CPython C extension for high-performance statistical operations.",
	-1,
	FastStatsMethods
};

/**
 * PyInit_fast_stats - initializes the fast_stats module
 *
 * Return: pointer to created PyModuleObject
 */
PyMODINIT_FUNC PyInit_c_python_pybaseline(void)
{
	return (PyModule_Create(&c_python_pybaseline_module));
}
