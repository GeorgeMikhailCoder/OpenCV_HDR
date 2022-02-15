// Test2.cpp: определяет точку входа для приложения.
//

#include "OpenCV_HDR.h"
#include<opencv2/opencv.hpp>
using namespace std;
using namespace cv;

vector<Mat> gaussPyram(const Mat& src, int depth = 5)
{
	vector<Mat> res;
	Mat buf(src);
	for (int i = 0; i < depth && (buf.rows > 2 && buf.cols > 2); i++)
	{
		res.push_back(buf);
		pyrDown(buf, buf);
	}
	return res;
}

vector<Mat> laplasPyram(const Mat& src, int depth = 5)
{
	vector<Mat> res;

	Mat buf_src(src);
	Mat buf_low(src);
	Mat buf_up(src);
	Mat buf_laplas(src);

	for (int i = 0; i < depth - 1 && (buf_src.rows > 2 && buf_src.cols > 2); i++)
	{
		buf_src = buf_low;
		pyrDown(buf_low, buf_low);
		pyrUp(buf_low, buf_up);
		res.push_back(buf_src - buf_up(Range(0, buf_src.rows), Range(0, buf_src.cols)));
	}
	res.push_back(buf_low);
	return res;
}

Mat mix(const Mat& src1, const Mat& src2, const Mat& mask)
{
	Mat res(src1);

	for (int y = 0; y < res.rows; y++)
		for (int x = 0; x < res.cols; x++)
		{
			//res.at<src1.type>
		}
	return Mat();
}

int main()
{
	Mat im = imread("../../../img/2.jpg", IMREAD_COLOR);
	if (im.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}
	imshow("im", im);

	vector<Mat> mass = laplasPyram(im, 4);
	Mat im2;
	im2 = mass[2];
	int a = im.type();
	im2.at<float>(0, 0);


	for (int i = 0; i < mass.size(); i++)
		imshow(to_string(i), mass[i]);
	imshow("im2", im2);
	waitKey();

	return 0;
}
