#include <omp.h>  
#include <stdio.h>  
#include <iostream>
#include <string>
#include <math.h>
#include <tuple>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <chrono> 
#include <ctime>
#include "mpi.h"

//F:\stuff\teme\Tema3_SOD\Tema3_SOD\Tema3_SOD\images\penguin.jpg

using namespace std;
using namespace cv;

#define threads 4
int nr_sums, instance_id, nr_procs;

bool is_root_instance()
{
	return instance_id == 0;
}

int get_root_instance_id()
{
	return 0;
}

int get_partial_pi_tag()
{
	return 666;
}

int get_main_tag()
{
	return 667;
}

bool string_ends_with(string const &fullString, string const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

void MPI_Broadcast_Text_To_Childs(string text) {
	char buffer[500];

	for (int i = 0; i < text.length(); i++) {
		buffer[i] = text[i];
	}

	buffer[text.length()] = 0;

	for (int i = 1; i < nr_procs; i++) {
		MPI_Send(buffer, 500, MPI_CHAR, i, get_main_tag(), MPI_COMM_WORLD);
	}
}

void MPI_Broadcast_Int_To_Childs(int value) {
	for (int i = 1; i < nr_procs; i++) {
		MPI_Send(&value, 1, MPI_INT, i, get_main_tag(), MPI_COMM_WORLD);
	}
}

void MPI_Broadcast_Double_To_Childs(double value) {
	for (int i = 1; i < nr_procs; i++) {
		MPI_Send(&value, 1, MPI_DOUBLE, i, get_main_tag(), MPI_COMM_WORLD);
	}
}

void MPI_Send_Int_To_Child(int value, int child_id) {
	MPI_Send(&value, 1, MPI_INT, child_id, get_main_tag(), MPI_COMM_WORLD);
}

void MPI_Send_Double_To_Child(double value, int child_id) {
	MPI_Send(&value, 1, MPI_DOUBLE, child_id, get_main_tag(), MPI_COMM_WORLD);
}

int MPI_Receive_Int_From_Parent() {
	int value;
	MPI_Status status;

	MPI_Recv(&value, 1, MPI_INT, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD, &status);

	return value;
}

double MPI_Receive_Double_From_Parent() {
	double value;
	MPI_Status status;

	MPI_Recv(&value, 1, MPI_DOUBLE, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD, &status);

	return value;
}

string MPI_Receive_Text_From_Parent() {
	char buffer[500];
	MPI_Status status;

	MPI_Recv(buffer, 500, MPI_CHAR, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD, &status);

	return buffer;
}

void MPI_Broadcast_Image_To_Childs(const Mat& image_to_broadcast) {
	uchar* buffer = new uchar[image_to_broadcast.rows * image_to_broadcast.cols * 3];

	for (int j = 0; j < image_to_broadcast.rows * image_to_broadcast.cols; j++) {
		int column = j % image_to_broadcast.cols;
		int row = j / image_to_broadcast.cols;
		Vec3b pixel = image_to_broadcast.at<Vec3b>(row, column);
		buffer[j * 3] = pixel[0];
		buffer[(j * 3) + 1] = pixel[1];
		buffer[(j * 3) + 2] = pixel[2];
	}

	for (int i = 1; i < nr_procs; i++) {
		int rows = image_to_broadcast.rows;
		int cols = image_to_broadcast.cols;

		MPI_Send(&rows, 1, MPI_INT, i, get_main_tag(), MPI_COMM_WORLD);
		MPI_Send(&cols, 1, MPI_INT, i, get_main_tag(), MPI_COMM_WORLD);
		MPI_Send(buffer, image_to_broadcast.rows * image_to_broadcast.cols * 3, MPI_UNSIGNED_CHAR, i, get_main_tag(), MPI_COMM_WORLD);
	}

	delete buffer;
}

void MPI_Send_Image_To_Parent(const Mat& image_to_send) {
	uchar* buffer = new uchar[image_to_send.rows * image_to_send.cols * 3];

	for (int j = 0; j < image_to_send.rows * image_to_send.cols; j++) {
		int column = j % image_to_send.cols;
		int row = j / image_to_send.cols;
		Vec3b pixel = image_to_send.at<Vec3b>(row, column);
		buffer[j * 3] = pixel[0];
		buffer[(j * 3) + 1] = pixel[1];
		buffer[(j * 3) + 2] = pixel[2];
	}

	int rows = image_to_send.rows;
	int cols = image_to_send.cols;

	MPI_Send(&rows, 1, MPI_INT, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD);
	MPI_Send(&cols, 1, MPI_INT, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD);
	MPI_Send(buffer, image_to_send.rows * image_to_send.cols * 3, MPI_UNSIGNED_CHAR, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD);

	delete buffer;
}

Mat MPI_Receive_Image_From_Parent() {
	int rows;
	int cols;
	MPI_Status status;

	MPI_Recv(&rows, 1, MPI_INT, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD, &status);
	MPI_Recv(&cols, 1, MPI_INT, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD, &status);

	uchar* buffer = new uchar[rows * cols * 3];

	MPI_Recv(buffer, rows * cols * 3, MPI_UNSIGNED_CHAR, get_root_instance_id(), get_main_tag(), MPI_COMM_WORLD, &status);

	Mat received_image(rows, cols, CV_8UC3);

	for (int j = 0; j < rows * cols; j++) {
		int column = j % cols;
		int row = j / cols;

		Vec3b pixel;
		pixel[0] = buffer[j * 3];
		pixel[1] = buffer[(j * 3) + 1];
		pixel[2] = buffer[(j * 3) + 2];

		received_image.at<Vec3b>(row, column) = pixel;
	}

	delete buffer;

	return received_image;
}

Mat MPI_Receive_Image_From_Child(int child_id) {
	int rows;
	int cols;
	MPI_Status status;

	MPI_Recv(&rows, 1, MPI_INT, child_id, get_main_tag(), MPI_COMM_WORLD, &status);
	MPI_Recv(&cols, 1, MPI_INT, child_id, get_main_tag(), MPI_COMM_WORLD, &status);

	uchar* buffer = new uchar[rows * cols * 3];

	MPI_Recv(buffer, rows * cols * 3, MPI_UNSIGNED_CHAR, child_id, get_main_tag(), MPI_COMM_WORLD, &status);

	Mat received_image(rows, cols, CV_8UC3);

	for (int j = 0; j < rows * cols; j++) {
		int column = j % cols;
		int row = j / cols;

		Vec3b pixel;
		pixel[0] = buffer[j * 3];
		pixel[1] = buffer[(j * 3) + 1];
		pixel[2] = buffer[(j * 3) + 2];

		received_image.at<Vec3b>(row, column) = pixel;
	}

	delete buffer;

	return received_image;
}

vector<tuple<int, int, int, int>> calculate_slices(int rows, int cols) {
	vector<tuple<int, int, int, int>> slices;
	int childs_count = nr_procs - 1;
	int slice_size = cols / childs_count;

	for (int i = 0; ; i += slice_size) {
		if (i + 2 * slice_size > cols) {
			slices.push_back(tuple<int, int, int, int>(0, rows, i, cols));

			break;
		}

		slices.push_back(tuple<int, int, int, int>(0, rows, i, i + slice_size));
	}

	return slices;
}

void MPI_Send_Slices_To_Childs(const vector<tuple<int, int, int, int>>& slices) {
	int childs_count = slices.size();

	for (int i = 0; i < childs_count; i++) {
		tuple<int, int, int, int> current_slice = slices[i];

		MPI_Send_Int_To_Child(get<0>(current_slice), i + 1);
		MPI_Send_Int_To_Child(get<1>(current_slice), i + 1);
		MPI_Send_Int_To_Child(get<2>(current_slice), i + 1);
		MPI_Send_Int_To_Child(get<3>(current_slice), i + 1);
	}
}

Mat MPI_Receive_Image_From_Childs(int width, int height, const vector<tuple<int, int, int, int>>& slices) {
	Mat image(height, width, CV_8UC3);
	int slices_size = slices.size();

	for (int i = 0; i < slices_size; i++) {
		Mat received_image = MPI_Receive_Image_From_Child(i + 1);
		tuple<int, int, int, int> current_slice = slices.at(i);

		for (int x = get<0>(current_slice); x < get<1>(current_slice); x++) {
			for (int y = get<2>(current_slice); y < get<3>(current_slice); y++) {
				Vec3b current_pixel = received_image.at<Vec3b>(x, y);
				image.at<Vec3b>(x, y) = current_pixel;
			}
		}
	}

	return image;
}


void save_image(Mat image_to_save, string path) {
	if (string_ends_with(path, ".png") || string_ends_with(path, ".PNG")) {

		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(6);

		imwrite(path, image_to_save, compression_params);
	}
	else if (string_ends_with(path, ".jpg") || string_ends_with(path, ".JPG") || string_ends_with(path, ".JPEG") || string_ends_with(path, ".jpeg")) {

		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
		compression_params.push_back(95);

		imwrite(path, image_to_save, compression_params);
	}
}

void convert_to_grayscale(string path, string new_path)
{
	if (is_root_instance()) {
		Mat image;
		image = imread(path, CV_LOAD_IMAGE_COLOR);

		MPI_Broadcast_Image_To_Childs(image);

		vector<tuple<int, int, int, int>> slices = calculate_slices(image.rows, image.cols);

		MPI_Send_Slices_To_Childs(slices);

		int width = image.cols;
		int height = image.rows;

		Mat grayscaled_image = MPI_Receive_Image_From_Childs(width, height, slices);

		save_image(grayscaled_image, new_path);
	}
	else {
		Mat image = MPI_Receive_Image_From_Parent();

		int width = image.cols;
		int height = image.rows;

		int top = MPI_Receive_Int_From_Parent();
		int bottom = MPI_Receive_Int_From_Parent();

		int left = MPI_Receive_Int_From_Parent();
		int right = MPI_Receive_Int_From_Parent();

		Mat grayscaled_image(height, width, CV_8UC3);
#pragma omp parallel num_threads(threads) 
		{
			if (width && height) {
#pragma omp for 
				for (int i = top; i < bottom; i++) {
					for (int j = left; j < right; j++) {
						Vec3b clrOriginal = image.at<Vec3b>(i, j);
						double fR(clrOriginal[0]);
						double fG(clrOriginal[1]);
						double fB(clrOriginal[2]);

						float fWB = sqrt((fR * fR + fG * fG + fB * fB) / 3);

						Vec3b new_color;
						new_color[0] = static_cast<uchar>(fWB);
						new_color[1] = static_cast<uchar>(fWB);
						new_color[2] = static_cast<uchar>(fWB);

						grayscaled_image.at<Vec3b>(i, j) = new_color;
					}
				}
			}
		}

#pragma omp barrier  

		MPI_Send_Image_To_Parent(grayscaled_image);
	}
}

void resize_image(string path, string new_path) {
	Mat image;
	double horizontal_resize_percentage;
	double vertical_resize_percentage;

	if (is_root_instance()) {
		image = imread(path, CV_LOAD_IMAGE_COLOR);


		cout << "Please enter horizontal resize percentage:" << endl;
		cin >> horizontal_resize_percentage;
		MPI_Broadcast_Int_To_Childs(horizontal_resize_percentage);

		cout << "Please enter vertical resize percentage:" << endl;
		cin >> vertical_resize_percentage;

		MPI_Broadcast_Int_To_Childs(vertical_resize_percentage);

		Mat image;
		image = imread(path, CV_LOAD_IMAGE_COLOR);

		MPI_Broadcast_Image_To_Childs(image);

		int resized_rows = static_cast<int>((image.rows * vertical_resize_percentage) / 100);
		int resized_columns = static_cast<int>((image.cols * horizontal_resize_percentage) / 100);

		vector<tuple<int, int, int, int>> slices = calculate_slices(resized_rows, resized_columns);

		MPI_Send_Slices_To_Childs(slices);

		Mat resized_image = MPI_Receive_Image_From_Childs(resized_columns, resized_rows, slices);

		save_image(resized_image, new_path);
	}
	else {
		horizontal_resize_percentage = MPI_Receive_Int_From_Parent();

		vertical_resize_percentage = MPI_Receive_Int_From_Parent();

		image = MPI_Receive_Image_From_Parent();

		int top = MPI_Receive_Int_From_Parent();
		int bottom = MPI_Receive_Int_From_Parent();

		int left = MPI_Receive_Int_From_Parent();
		int right = MPI_Receive_Int_From_Parent();

		int resized_rows = static_cast<int>((image.rows * vertical_resize_percentage) / 100);
		int resized_columns = static_cast<int>((image.cols * horizontal_resize_percentage) / 100);

		int initial_columns = image.cols;
		int initial_rows = image.rows;

		Size new_size(resized_columns, resized_rows);
		Mat resized_image(new_size, CV_8UC3);


#pragma omp parallel num_threads(threads)  
		{
#pragma omp for 
			for (int current_row = top; current_row < bottom; current_row++) {
				for (int current_column = left; current_column < right; current_column++) {

					double horizontal_projection_center = (current_column * 100) / horizontal_resize_percentage;
					double vertical_projection_center = (current_row * 100) / vertical_resize_percentage;

					vector<tuple<int, float>> unbounded_horizontal_indices_weights;
					vector<tuple<int, float>> unbounded_vertical_indices_weights;

					int left_limit = 0;
					int right_limit = 0;
					int bottom_limit = 0;
					int top_limit = 0;

					if (horizontal_resize_percentage > 100) {
						tuple<int, float> left_limit(floor(horizontal_projection_center), 1 - abs(floor(horizontal_projection_center) - horizontal_projection_center));
						tuple<int, float> right_limit(ceill(horizontal_projection_center), 1 - abs(ceill(horizontal_projection_center) - horizontal_projection_center));

						unbounded_horizontal_indices_weights.push_back(left_limit);
						unbounded_horizontal_indices_weights.push_back(right_limit);
					}
					else {
						double horizontal_distance = horizontal_resize_percentage / 100;

						tuple<int, float> left_limit(floor(horizontal_projection_center - horizontal_distance), 1 - abs(floor(horizontal_projection_center - horizontal_distance) - (horizontal_projection_center - horizontal_distance)));
						tuple<int, float> right_limit(ceill(horizontal_projection_center + horizontal_distance), 1 - abs(ceill(horizontal_projection_center + horizontal_distance) - (horizontal_projection_center + horizontal_distance)));

						unbounded_horizontal_indices_weights.push_back(left_limit);
						unbounded_horizontal_indices_weights.push_back(right_limit);

						for (int i = floor(horizontal_projection_center - horizontal_distance) + 1; i <= ceill(horizontal_projection_center + horizontal_distance) - 1; i++) {
							tuple<int, float> current_weight(i, 1);

							unbounded_horizontal_indices_weights.push_back(current_weight);
						}
					}

					if (vertical_resize_percentage > 100) {
						tuple<int, float> top_limit(floor(vertical_projection_center), 1 - abs(floor(vertical_projection_center) - vertical_projection_center));
						tuple<int, float> bottom_limit(ceill(vertical_projection_center), 1 - abs(ceill(vertical_projection_center) - vertical_projection_center));

						unbounded_vertical_indices_weights.push_back(top_limit);
						unbounded_vertical_indices_weights.push_back(bottom_limit);
					}
					else {
						double vertical_distance = vertical_resize_percentage / 100;

						tuple<int, float> top_limit(floor(vertical_projection_center - vertical_distance), 1 - abs(floor(vertical_projection_center - vertical_distance) - (vertical_projection_center - vertical_distance)));
						tuple<int, float> bottom_limit(ceill(vertical_projection_center + vertical_distance), 1 - abs(ceill(vertical_projection_center + vertical_distance) - (horizontal_projection_center + vertical_distance)));

						unbounded_vertical_indices_weights.push_back(top_limit);
						unbounded_vertical_indices_weights.push_back(bottom_limit);

						for (int i = floor(vertical_projection_center - vertical_distance) + 1; i <= ceill(vertical_projection_center + vertical_distance) - 1; i++) {
							tuple<int, float> current_weight(i, 1);

							unbounded_vertical_indices_weights.push_back(current_weight);
						}
					}

					vector<tuple<int, float>> horizontal_indices_weights;
					vector<tuple<int, float>> vertical_indices_weights;

					for (int index_indices = 0; index_indices < unbounded_horizontal_indices_weights.size(); index_indices++) {
						tuple<int, float> current_weight = unbounded_horizontal_indices_weights.at(index_indices);

						if (get<0>(current_weight) >= 0 && get<0>(current_weight) < initial_columns) {
							horizontal_indices_weights.push_back(current_weight);
						}
					}

					for (int index_indices = 0; index_indices < unbounded_vertical_indices_weights.size(); index_indices++) {
						tuple<int, float> current_weight = unbounded_vertical_indices_weights.at(index_indices);

						if (get<0>(current_weight) >= 0 && get<0>(current_weight) < initial_rows) {
							vertical_indices_weights.push_back(current_weight);
						}
					}

					double total_weights = 0;

					for (int index_in_horizontal = 0; index_in_horizontal < horizontal_indices_weights.size(); index_in_horizontal++) {
						for (int index_in_vertical = 0; index_in_vertical < vertical_indices_weights.size(); index_in_vertical++) {
							tuple<int, float> horizontal_weight = horizontal_indices_weights.at(index_in_horizontal);
							tuple<int, float> vetical_weight = vertical_indices_weights.at(index_in_vertical);

							total_weights += get<1>(horizontal_weight) * get<1>(vetical_weight);
						}
					}

					double average_red = 0;
					double average_green = 0;
					double average_blue = 0;

					double normalize_delta = 255 / 2;

					for (int index_in_horizontal = 0; index_in_horizontal < horizontal_indices_weights.size(); index_in_horizontal++) {
						for (int index_in_vertical = 0; index_in_vertical < vertical_indices_weights.size(); index_in_vertical++) {
							tuple<int, float> horizontal_weight = horizontal_indices_weights.at(index_in_horizontal);
							tuple<int, float> vertical_weight = vertical_indices_weights.at(index_in_vertical);

							Vec3b colors = image.at<Vec3b>(get<0>(vertical_weight), get<0>(horizontal_weight));

							average_blue += (colors[0] - normalize_delta)  * get<1>(horizontal_weight) * get<1>(vertical_weight);

							average_green += (colors[1] - normalize_delta)  * get<1>(horizontal_weight) * get<1>(vertical_weight);

							average_red += (colors[2] - normalize_delta)  * get<1>(horizontal_weight) * get<1>(vertical_weight);
						}
					}

					average_blue = (average_blue / total_weights) + normalize_delta;
					average_green = (average_green / total_weights) + normalize_delta;
					average_red = (average_red / total_weights) + normalize_delta;

					Vec3b new_color;

					new_color[0] = static_cast<uchar>(average_blue);
					new_color[1] = static_cast<uchar>(average_green);
					new_color[2] = static_cast<uchar>(average_red);

					resized_image.at<Vec3b>(current_row, current_column) = new_color;
				}
			}
		}

#pragma omp barrier  

		MPI_Send_Image_To_Parent(resized_image);
	}
}

void rotate_image(string path, string new_path) {
	string operation;
	Mat image;
	int direction;
	image = imread(path, CV_LOAD_IMAGE_COLOR);

	cout << "Please enter direction to rotate" << endl;
	cout << "1. Right" << endl;
	cout << "2. Left" << endl;

	cin >> operation;

	if (operation == "1" || operation == "Right") {
		direction = 1;
	}
	else {
		direction = -1;
	}

	Mat rotated_image(image.cols, image.rows, CV_8UC3);

	int columns = image.cols;
	int rows = image.rows;
	int total_pixels = columns * rows;

	if (!is_root_instance()) {
		return;
	}

	if (direction == 1) {
#pragma omp parallel num_threads(threads) 
		{
#pragma omp for 
			for (int index = 0; index < total_pixels; index++) {
				int current_row = index / columns;
				int current_column = index % columns;

				Vec3b rotated_pixel;
				Vec3b original_pixel = image.at<Vec3b>(current_row, current_column);

				rotated_pixel[0] = original_pixel[0];
				rotated_pixel[1] = original_pixel[1];
				rotated_pixel[2] = original_pixel[2];

				rotated_image.at<Vec3b>(current_column, rows - 1 - current_row) = rotated_pixel;
			}
		}

		save_image(rotated_image, new_path);
	}
	else {
#pragma omp parallel num_threads(threads)
		{
#pragma omp for 
			for (int index = 0; index < total_pixels; index++) {
				int current_row = index / columns;
				int current_column = index % columns;

				Vec3b rotated_pixel;
				Vec3b original_pixel = image.at<Vec3b>(current_row, current_column);

				rotated_pixel[0] = original_pixel[0];
				rotated_pixel[1] = original_pixel[1];
				rotated_pixel[2] = original_pixel[2];

				rotated_image.at<Vec3b>(columns - current_column - 1, current_row) = rotated_pixel;
			}
		}

#pragma omp barrier  

		save_image(rotated_image, new_path);
	}
}

void blur_image(string path, string new_path) {
	Mat image;
	int columns;
	int rows;
	int total_pixels;
	double blur_radius;

	if (is_root_instance()) {

		Mat image;
		image = imread(path, CV_LOAD_IMAGE_COLOR);
		MPI_Broadcast_Image_To_Childs(image);

		double blur_radius = 0;

		cout << "Please enter the blur radius" << endl;
		cin >> blur_radius;

		int width = image.cols;
		int height = image.rows;

		int columns = width;
		int rows = height;
		int total_pixels = columns * rows;

		MPI_Broadcast_Double_To_Childs(blur_radius);
		MPI_Broadcast_Int_To_Childs(columns);
		MPI_Broadcast_Int_To_Childs(rows);
		MPI_Broadcast_Int_To_Childs(total_pixels);

		vector<tuple<int, int, int, int>> slices = calculate_slices(image.rows, image.cols);
		MPI_Send_Slices_To_Childs(slices);

		Mat blurred_image = MPI_Receive_Image_From_Childs(width, height, slices);

		save_image(blurred_image, new_path);
	}
	else {
		image = MPI_Receive_Image_From_Parent();
		blur_radius = MPI_Receive_Double_From_Parent();
		columns = MPI_Receive_Int_From_Parent();
		rows = MPI_Receive_Int_From_Parent();
		total_pixels = MPI_Receive_Int_From_Parent();

		int top_limit = MPI_Receive_Int_From_Parent();
		int bottom_limit = MPI_Receive_Int_From_Parent();

		int left_limit = MPI_Receive_Int_From_Parent();
		int right_limit = MPI_Receive_Int_From_Parent();

		Size new_size(columns, rows);
		Mat blurred_image(new_size, CV_8UC3);


#pragma omp parallel num_threads(threads)
		{
#pragma omp for 
			for (int x = left_limit; x < right_limit; x++) {
				for (int y = top_limit; y < bottom_limit; y++)
				{
					int current_row = y;
					int current_column = x;

					double left = current_column - blur_radius;
					double right = current_column + blur_radius + 0.000001;
					double top = current_row - blur_radius;
					double bottom = current_row + blur_radius + 0.00001;

					if (top < 0) {
						top = 0;
					}

					if (bottom > rows) {
						bottom = rows - 1;
					}

					if (left < 0) {
						left = 0;
					}

					if (right > columns) {
						right = columns - 1;
					}

					double total_weight = 0;
					double total_r = 0;
					double total_g = 0;
					double total_b = 0;

					for (double i = top; i <= bottom; i++) {
						for (double j = left; j < right; j++) {
							int column = floor(j);
							int row = floor(i);

							double horizontal_weight = (blur_radius + 1) - abs(current_column - j + 1);
							double vertical_weight = (blur_radius + 1) - abs(current_row - i + 1);
							double current_weight = horizontal_weight * vertical_weight;
							Vec3b colors = image.at<Vec3b>(i, j);

							total_weight += horizontal_weight * vertical_weight;

							total_r += current_weight * colors[0];
							total_g += current_weight * colors[1];
							total_b += current_weight * colors[2];
						}
					}

					Vec3b total_colors;

					total_colors[0] = total_r / total_weight;
					total_colors[1] = total_g / total_weight;
					total_colors[2] = total_b / total_weight;

					blurred_image.at<Vec3b>(current_row, current_column) = total_colors;
				}
			}
		}

#pragma omp barrier  

		MPI_Send_Image_To_Parent(blurred_image);
	}
}


int main(int argc, char* argv[])
{
	string input, path, operation, new_path;

	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &instance_id);
	MPI_Comm_size(MPI_COMM_WORLD, &nr_procs);

	while (true) {
		if (is_root_instance())
		{
			cout << "Do you want to execute another action:" << endl;
			cout << "1. Yes" << endl;
			cout << "2. No" << endl;

			cin >> input;

			MPI_Broadcast_Text_To_Childs(input);

			if (input == "2" || input == "No") {
				break;
			}

			cout << "Please enter the path to the image:" << endl;
			cin >> path;

			cout << "Please enter the path to save the new image:" << endl;
			cin >> new_path;

			cout << "Please enter the operation to perform on the image:" << endl;
			cout << "1. Resize" << endl;
			cout << "2. Rotate" << endl;
			cout << "3. Grayscale" << endl;
			cout << "4. Blur" << endl;
			cin >> operation;


			// Record start time
			auto start = clock();

			MPI_Broadcast_Text_To_Childs(operation);

			if (operation == "1" || operation == "Resize") {
				resize_image(path, new_path);
			}
			else if (operation == "2" || operation == "Rotate") {
				rotate_image(path, new_path);
			}
			else if (operation == "3" || operation == "Grayscale") {
				convert_to_grayscale(path, new_path);
			}
			else if (operation == "4" || operation == "Blur") {
				blur_image(path, new_path);
			}

			auto finish = clock();

			auto elapsed_tine = double(finish - start) / CLOCKS_PER_SEC;

			cout << "Elapsed time for operation: " << elapsed_tine << endl;
		}
		else {
			input = MPI_Receive_Text_From_Parent();

			if (input == "2" || input == "No") {
				break;
			}

			operation = MPI_Receive_Text_From_Parent();

			if (operation == "1" || operation == "Resize") {
				resize_image(path, new_path);
			}
			else if (operation == "2" || operation == "Rotate") {
				rotate_image(path, new_path);
			}
			else if (operation == "3" || operation == "Grayscale") {
				convert_to_grayscale(path, new_path);
			}
			else if (operation == "4" || operation == "Blur") {
				blur_image(path, new_path);
			}
		}
	}
}

