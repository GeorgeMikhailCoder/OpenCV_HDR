// Test2.cpp: определяет точку входа для приложения.
//

#include "OpenCV_HDR.h"
#include<opencv2/opencv.hpp>
using namespace std;
using namespace cv;

vector<Mat> gaussPyram(const Mat& src, int depth = 7)
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

vector<Mat> laplasPyram(const Mat& src, int depth = 7)
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
	//assert(src1.depth() == 0 && src2.depth() ==0  && mask.depth() == 0);
	assert(src1.depth() == 5 && src2.depth() == 5 && mask.depth() == 5);
	using curType = float;

	assert(src1.rows == src2.rows && src2.rows == mask.rows);
	assert(src1.cols == src2.cols && src2.cols == mask.cols);
	assert(src1.channels() == src2.channels() && src2.channels() == mask.channels());
	

	vector<Mat>planes1;
	split(src1, planes1);

	vector<Mat>planes2;
	split(src2, planes2);

	vector<Mat>planesMask;
	split(mask, planesMask);

	vector<Mat> vec;
	

	for (int ch = 0; ch < mask.channels(); ch++)
	{
		Mat resChannel(mask.rows, mask.cols, mask.depth());
		curType maxPixel = mask.depth() == 0 ? 255 : 1.0;

		for (int y = 0; y < resChannel.rows; y++)
			for (int x = 0; x < resChannel.cols; x++)
			{
				const curType u1 = src1.at<curType>(y, x);
				const curType u2 = src2.at<curType>(y, x);
				const curType m = mask.at<curType>(y, x);
				resChannel.at<curType>(y, x) = (curType)((u1 * (maxPixel - m) + u2 * m) / maxPixel);
			}
		vec.push_back(resChannel);
	}
	Mat res;
	merge(vec, res);
	return res;
}

Mat laplasPyramInverse(const vector<Mat> pyram)
{
	int depth = (int)pyram.size() - 1;
	Mat uk = pyram[depth];

	Mat lk;
	int i;
	for (i = depth - 1; i >= 0; i--)
	{
		lk = pyram[i];
		pyrUp(uk,uk);
		uk = lk + uk(Range(0,lk.rows),Range(0,lk.cols));
	}

	return uk;
}

vector<Mat> mixPyram(const vector<Mat> mass1, const vector<Mat> mass2, const vector<Mat> mask)
{
	assert(mass1.size() == mass2.size() && mass2.size() == mask.size());
	
	vector<Mat> res(mass1.size());
	for (int i = 0; i < mass1.size(); i++)
		res[i] = mix(mass1[i], mass2[i], mask[i]);
	return res;
}

void printPyram(vector<Mat> mass)
{
	for (int i = 0; i < mass.size(); i++)
		imshow(to_string(i), mass[i]);
}

Mat mixHDR(const Mat& im1, const Mat& im2, const Mat& mask)
{
	vector<Mat> maskMass = gaussPyram(mask, 100);
	int dp = (int)maskMass.size();
	
	vector<Mat> mass1 = laplasPyram(im1, dp);
	vector<Mat> mass2 = laplasPyram(im2, dp);

	vector<Mat> res = mixPyram(mass1, mass2, maskMass);

	Mat dst = laplasPyramInverse(res);
	return dst;
}

Mat toFloatVec(const Mat& src)
{
	vector<Mat> mass1, mass11;
	split(src, mass1);
	Mat im1;
	for (int ch = 0; ch < src.channels(); ch++)
	{
		Mat tmp;
		mass1[ch].convertTo(tmp, CV_32F);
		mass11.push_back(tmp / 255);
	}
	merge(mass11, im1);
	return im1;
}

Mat toFloat(const Mat& src)
{
	Mat res;
	src.convertTo(res, CV_32FC3);
	res /= 255;
	return res;
}


int main()
{
	Mat im11 = imread("../../../img/hulk1.jpg", IMREAD_COLOR);
	if (im11.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}
	
	Mat im22 = imread("../../../img/smith1.jpg", IMREAD_COLOR);
	if (im22.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}

	Mat mask1 = imread("../../../img/mask_sh1.jpg", IMREAD_COLOR);
	if (mask1.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}

	Mat im1 = toFloat(im11);
	Mat im2 = toFloat(im22);
	Mat mask = toFloat(mask1);

	imshow("src1", im1);
	imshow("src2", im2);
	imshow("mask", mask);

	// Mat dst = mixHDR(im1, im2, mask);

	Mat dst = im1;
	vector<Mat> tmp;
	tmp = laplasPyram(dst);
	dst = laplasPyramInverse(tmp);
	imshow("res",dst);

	waitKey();
	system("pause");
	return 0;
}
