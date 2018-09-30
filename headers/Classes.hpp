/*
 * Classes.h
 *
 *  Created on: 4 Dec 2017
 *      Author: detlevcm
 */

#ifndef HEADERS_CLASSES_HPP_
#define HEADERS_CLASSES_HPP_

class FileNames {
public:
	/*
	 * This class collects the file names for the
	 * input files required during a run.
	 *
	 * To collate data, output files are equally treated in the
	 * same class, together together potentially with output flags.
	 */

	string mechanism;
	string initial_data;
	string species_mapping;

};






#endif /* HEADERS_CLASSES_HPP_ */