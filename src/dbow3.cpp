// Author: Andrius Mikonis (andrius.mikonis@gmail.com)
// License: BSD
// Last modified: Feb 12, 2019

// Wrapper for most external modules
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <exception>

// Opencv includes
#include <opencv2/opencv.hpp>

// np_opencv_converter
#include "np_opencv_converter.hpp"

// DBoW3
#include "DBoW3.h"

namespace py = boost::python;

cv::Mat test_np_mat(const cv::Mat& in) {
	std::cerr << "in: " << in << std::endl;
	std::cerr << "sz: " << in.size() << std::endl;
	return in.clone();
}

cv::Mat test_with_args(const cv::Mat_<float>& in, const int& var1 = 1,
	const double& var2 = 10.0, const std::string& name = std::string("test_name")) {
	std::cerr << "in: " << in << std::endl;
	std::cerr << "sz: " << in.size() << std::endl;
	std::cerr << "Returning transpose" << std::endl;
	return in.t();
}

class GenericWrapper {
public:
	GenericWrapper(const int& _var_int = 1, const float& _var_float = 1.f,
		const double& _var_double = 1.f, const std::string& _var_string = std::string("test_string"))
		: var_int(_var_int), var_float(_var_float), var_double(_var_double), var_string(_var_string)
	{

	}

	cv::Mat process(const cv::Mat& in) {
		std::cerr << "in: " << in << std::endl;
		std::cerr << "sz: " << in.size() << std::endl;
		std::cerr << "Returning transpose" << std::endl;
		return in.t();
	}

private:
	int var_int;
	float var_float;
	double var_double;
	std::string var_string;
};


class Vocabulary
{
public:
	Vocabulary(int k = 10, int L = 5, DBoW3::WeightingType weighting = DBoW3::TF_IDF, DBoW3::ScoringType scoring = DBoW3::L1_NORM, const std::string& path = std::string()) {
		vocabulary = new DBoW3::Vocabulary(k, L, weighting, scoring);
		if (!path.empty())
			load(path);
	}
	~Vocabulary() {
		delete vocabulary;
	}

	void create(const  std::vector<cv::Mat>   &training_features) {
		vocabulary->create(training_features);
	}

	void clear() {
		vocabulary->clear();
	}

	void load(const std::string& path) {
		vocabulary->load(path);
	}

	void save(const std::string& path, bool binary_compressed = true) {
		vocabulary->save(path, binary_compressed);
	}

//	DBoW3::BowVector transform(const  std::vector<cv::Mat> & features) {
	cv::Mat transform(const  std::vector<cv::Mat> & features) {
		DBoW3::BowVector word;
		std::cout << "Will process " << features.size() << " features, each "
			<< features[0].rows << ", " << features[0].cols << " in size. " << std::endl;
		vocabulary->transform(features, word);

		std::cout << "Computed BoW vector:" << std::endl;
		for (const auto &el : word) {
			std::cout << el.first << ", " << el.second << std::endl;
		}
		std::cout << word << std::endl;

		return cv::Mat(10, 10, CV_8UC1);
//		return word;
	}

	double score(const  DBoW3::BowVector &A, const DBoW3::BowVector &B) {
		return vocabulary->score(A, B);
	}

	unsigned int size() const {
		return vocabulary->size();
	}

	DBoW3::Vocabulary * vocabulary;
};

class Database
{
public:
	Database(const std::string& path = std::string()) {
		if (path.empty())
			database = new DBoW3::Database();
		else
			database = new DBoW3::Database(path);
	}
	~Database() {
		delete database;
	}

	void setVocabulary(const Vocabulary& vocabulary, bool use_di, int di_levels=0) {
		database->setVocabulary(*vocabulary.vocabulary, use_di, di_levels);
	}

	unsigned int add(const  cv::Mat & features) {
		return database->add(features, NULL, NULL);
	}

	/**
	 * @brief Adds the features to the database and returns the BoW representation of the features.
	 * @param features A single image's ORB features.
	 * @return A tuple consisting of the list of words and their weights. It consists in a cv::Mat1i of 1 row and F
	 * 		   columns and a cv::Mat1f of 1 row and F columns. F is at most the total number of features passed as
	 * 		   input, but can be smaller in the (likely) event that multiple features map to the same visual word.
	 */
	py::tuple addAndGetBow(const cv::Mat &features) {
		assert(features.rows == 1);
		DBoW3::BowVector bow_vector;
		this->database->add(features, &bow_vector, nullptr);

		// Pack the bow_vector, which is basically a map, into a list of keys and values.
		cv::Mat1i out_indices(1, bow_vector.size());
		cv::Mat1f out_weights(1, bow_vector.size());
		int idx = 0;
		for (auto const &kv : bow_vector) {
			out_indices.at<int>(idx) = kv.first;
			out_weights.at<float>(idx) = kv.second;
			idx++;
		}

		return py::make_tuple(out_indices, out_weights);
	}

	py::list multiAddAndGetBow(const cv::Mat &features) {
	  	// XXX(andreib): Populate this placeholder. Important for performance.
		std::vector<int> dummy = {1, 2, 3};
		return py::list(dummy);

	}

	std::vector<DBoW3::Result> query(const  cv::Mat &features, int max_results = 1, int max_id = -1) {
		DBoW3::QueryResults results;
		database->query(features, results, max_results, max_id);
		return results;
	}

	void save(const std::string &filename) const {
		database->save(filename);
	}

	void load(const std::string &filename) {
		database->load(filename);
	}

	void loadVocabulary(const std::string &filename, bool use_di, int di_levels=0) {
		DBoW3::Vocabulary voc;
		voc.load(filename);
		database->setVocabulary(voc, use_di, di_levels);
	}


private:
	DBoW3::Database * database;
};

// Wrap a few functions and classes for testing purposes
namespace fs {
	namespace python {

		BOOST_PYTHON_MODULE(pyDBoW3)
		{
			// Main types export
			fs::python::init_and_export_converters();
			py::scope scope = py::scope();

			py::enum_<DBoW3::WeightingType>("WeightingType")
				.value("TF_IDF", DBoW3::TF_IDF)
				.value("TF", DBoW3::TF)
				.value("IDF", DBoW3::IDF)
				.value("BINARY", DBoW3::BINARY);

			py::enum_<DBoW3::ScoringType>("ScoringType")
				.value("L1_NORM", DBoW3::L1_NORM)
				.value("L2_NORM", DBoW3::L2_NORM)
				.value("CHI_SQUARE", DBoW3::CHI_SQUARE)
				.value("KL", DBoW3::KL)
				.value("BHATTACHARYYA", DBoW3::BHATTACHARYYA)
				.value("DOT_PRODUCT", DBoW3::DOT_PRODUCT);

			// Class
			py::class_<Vocabulary>("Vocabulary")
				.def(py::init<py::optional<int, int, DBoW3::WeightingType, DBoW3::ScoringType, std::string> >(
				(py::arg("k") = 10, py::arg("L") = 5, py::arg("weighting") = DBoW3::TF_IDF, py::arg("scoring") = DBoW3::L1_NORM,
					py::arg("path") = std::string())))
				.def("load", &Vocabulary::load)
				.def("save", &Vocabulary::save)
				.def("create", &Vocabulary::create)
				.def("transform", &Vocabulary::transform, py::return_value_policy<py::return_by_value>())
				.def("score", &Vocabulary::score)
				.def("clear", &Vocabulary::clear)
				.def("size", &Vocabulary::size);

			py::class_<Database>("Database")
				.def(py::init<py::optional<std::string> >(py::arg("path") = std::string()))
				.def("setVocabulary", &Database::setVocabulary)
				.def("save", &Database::save)
				.def("load", &Database::load)
				.def("loadVocabulary", &Database::loadVocabulary)
				.def("add", &Database::add)
				.def("addAndGetBow", &Database::addAndGetBow, py::return_value_policy<py::return_by_value>())
				.def("query", &Database::query, py::return_value_policy<py::return_by_value>());

			py::class_<DBoW3::Result>("Result")
				.def_readonly("Id", &DBoW3::Result::Id)
				.def_readonly("Score", &DBoW3::Result::Score)
				.def_readonly("nWords", &DBoW3::Result::nWords)
				.def_readonly("bhatScore", &DBoW3::Result::bhatScore)
				.def_readonly("chiScore", &DBoW3::Result::chiScore)
				.def_readonly("sumCommonVi", &DBoW3::Result::sumCommonVi)
				.def_readonly("sumCommonWi", &DBoW3::Result::sumCommonWi)
				.def_readonly("expectedChiScore", &DBoW3::Result::expectedChiScore);
		}

	} // namespace fs
} // namespace python
