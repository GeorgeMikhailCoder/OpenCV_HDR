// Test2.cpp: определяет точку входа для приложения.
//

#include "OpenCV_HDR.h"
#include<opencv2/opencv.hpp>
using namespace std;
using namespace cv;

void printMinMax(Mat src)
{
	double minB, maxB;
	minMaxLoc(src, &minB, &maxB);
	cout << "min = " << minB << ", max = " << maxB << endl;
}

vector<Mat> gaussPyram(const Mat& src, int depth = -1)
{
	vector<Mat> res;
	Mat buf(src);
	for (int i = 0; i < uint(depth-1) && (buf.rows > 2 && buf.cols > 2); i++)
	{
		res.push_back(buf);
		pyrDown(buf, buf);
	}
	res.push_back(buf);
	return res;
}

vector<Mat> laplasPyram(const Mat& src, int depth = -1)
{
	vector<Mat> res;

	Mat buf_src(src);
	Mat buf_low(src);
	Mat buf_up;
	Mat buf_laplas;

	for (int i = 0; i < uint(depth-1) && (buf_low.rows > 2 && buf_low.cols > 2); i++)
	{
		buf_src = buf_low;
		pyrDown(buf_low, buf_low);
		pyrUp(buf_low, buf_up, Size(buf_src.cols, buf_src.rows));
		buf_laplas = buf_src - buf_up;
		printMinMax(buf_laplas);
		res.push_back(buf_laplas);
	}
	res.push_back(buf_low);
	return res;
}

Mat mix(const Mat& src1, const Mat& src2, const Mat& mask)
{
	//  assert(src1.depth() == 0 && src2.depth() ==0  && mask.depth() == 0);
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
		Mat src11 = planes1[ch];
		Mat src22 = planes2[ch];
		Mat mask1 = planesMask[ch];

		Mat resChannel(mask.rows, mask.cols, mask.depth());
		curType maxPixel = mask.depth() == 0 ? 255 : 1.0;

		for (int y = 0; y < resChannel.rows; y++)
			for (int x = 0; x < resChannel.cols; x++)
			{
				const curType u1 = src11.at<curType>(y, x);
				const curType u2 = src22.at<curType>(y, x);
				const curType m = mask1.at<curType>(y, x);
				curType uk = (curType)((u1 * (maxPixel - m) + u2 * m) / (maxPixel));
				uk = uk > maxPixel ? maxPixel : uk < 0 ? 0 : uk;
				resChannel.at<curType>(y, x) = uk;
			}

		vec.push_back(resChannel);
	}
	Mat res;
	merge(vec, res);
	return res;
}

vector<Mat>  laplasPyramInverse(const vector<Mat> pyram)
{
	int depth = (int)pyram.size() - 1;
	Mat uk = pyram[depth];

	Mat lk, tmp;
	int i;
	vector<Mat> res;
	res.push_back(uk);
	for (i = depth - 1; i >= 0; i--)
	{
		lk = pyram[i];
		pyrUp(uk, uk, Size(lk.cols, lk.rows));
		uk = lk + uk;
		res.push_back(uk);
	}
	
	return res;
}

vector<Mat> mixPyram(const vector<Mat> mass1, const vector<Mat> mass2, const vector<Mat> mask)
{
	assert(mass1.size() == mass2.size() && mass2.size() == mask.size());
	
	vector<Mat> res;
	for (int i = 0; i < mass1.size(); i++)
		res.push_back(mix(mass1[i], mass2[i], mask[i]));
	return res;
}

void printPyram(vector<Mat> mass)
{
	string id = to_string(rand() % 100) + "__";
	for (int i = 0; i < mass.size(); i++)
		imshow(id + to_string(i), mass[i]);
}

vector<Mat>  mixHDR(const Mat& im1, const Mat& im2, const Mat& mask)
{
	vector<Mat> maskMass = gaussPyram(mask);
	vector<Mat> mass1    = laplasPyram(im1);
	vector<Mat> mass2    = laplasPyram(im2);
	
	vector<Mat> res = mixPyram(mass1, mass2, maskMass);

	vector<Mat>  dst = laplasPyramInverse(res);
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

bool myCompareMat(const Mat& m1, const Mat& m2)
{
	Mat res;
	absdiff(m1, m2, res);
	//cout << res << endl;
	double minEl, maxEl;
	minMaxLoc(res, &minEl, &maxEl);
	cout << "min = " << minEl << ", max = " << maxEl << endl;
	return maxEl < 0.0001;
}

bool test_Pyram(Mat src)
{
	return myCompareMat(src, laplasPyramInverse(laplasPyram(src,5))[4]);
}

void preprocessSmith(Mat& im)
{
	vector<Mat> mass;
	split(im, mass);

	mass[1] = mass[2]/2;

	merge(mass, im);
}

Mat getHDRMask(Mat im1, Mat im2)
{
	assert(im1.rows == im2.rows);
	assert(im1.cols == im2.cols);
	assert(im1.channels() == im2.channels() && im2.channels() == 3);

	cvtColor(im1, im1, COLOR_BGR2GRAY);
	cvtColor(im2, im2, COLOR_BGR2GRAY);

	int porog = 128;

	Mat res(im1.rows, im1.cols, CV_8U);
	for (int y = 0; y < im1.rows; y++)
		for (int x = 0; x < im1.cols; x++)
		{
			const uchar u1 = im1.at<uchar>(y, x);
			const uchar u2 = im2.at<uchar>(y, x);
			res.at<uchar>(y, x) = abs(u1 - porog) < abs(u2 - porog) ? 0 : 255;
		}
	Mat tmp;
	vector<Mat> vec = { res,res,res };
	merge(vec, tmp);
	return tmp;
}

void mixSmulk()
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
	//preprocessSmith(im22);
	if (im22.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}

	Mat mask1 = imread("../../../img/mask_sh3.jpg", IMREAD_COLOR);
	if (mask1.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}

	Mat im1 = im11;
	Mat im2 = im22;
	Mat mask = mask1;

	im1 = toFloat(im11);
	im2 = toFloat(im22);
	mask = toFloat(mask1);

	//  imshow("test_laplas", laplasPyramInverse(laplasPyram(im1,5))[4]);

	printMinMax(im1);
	printMinMax(im2);
	printMinMax(mask);

	imshow("src1", im1);
	imshow("src2", im2);
	imshow("mask", mask);

	cout << "test " << test_Pyram(im1) << endl;

	vector<Mat>  dst = mixHDR(im1, im2, mask);

	printMinMax(dst[dst.size() - 1]);
	Mat res = dst[dst.size() - 1];
	printMinMax(res);
	double minB, maxB;
	minMaxLoc(res, &minB, &maxB);
	Mat res2 = Mat((res - minB) / (maxB - minB));

	imshow("res", res);
	Mat saver;
	cvtColor(res, saver, CV_8U);
	printMinMax(saver);
	normalize(saver, saver, 0, 255, NORM_MINMAX, CV_8U);
	printMinMax(saver);
	imwrite("../../../img/smulk1.jpg", saver);
	waitKey();
}

int main()
{
	Mat im11 = imread("../../../img/LDR_015_low.jpg", IMREAD_COLOR);
	if (im11.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}

	Mat im22 = imread("../../../img/LDR_015_high.jpg", IMREAD_COLOR);
	if (im22.empty())
	{
		cout << "Can't read image" << endl;
		char c;
		cin >> c;
		exit(0);
	}
	

	Mat mask1 = getHDRMask(im11, im22);

	Mat im1 = im11;
	Mat im2 = im22;
	Mat mask = mask1;

	im1 = toFloat(im11);
	im2 = toFloat(im22);
	mask = toFloat(mask1);

	imshow("src1", im1);
	imshow("src2", im2);
	imshow("mask", mask);
	
	vector<Mat>  dst = mixHDR(im1, im2, mask);
	Mat res = dst[dst.size() - 1];
	imshow("res", res);
	Mat saver;
	cvtColor(res, saver, CV_8U);
	normalize(saver, saver, 0, 255, NORM_MINMAX, CV_8U);
	imwrite("../../../img/HDR.jpg", saver);
	waitKey();

	system("pause");
	return 0;
}
