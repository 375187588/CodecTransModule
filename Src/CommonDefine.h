#pragma once

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavdevice\avdevice.h"
#include "libavfilter\avfilter.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
#include "libavutil\imgutils.h"
#include "SDL.h"
}

#if CONFIG_AVFILTER
# include "libavfilter/avcodec.h"
# include "libavfilter/avfilter.h"
# include "libavfilter/avfiltergraph.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif

#include <deque>

#define MAX_BUFFER_SIZE 100

struct stDataLength
{
	unsigned int nDataLength;
};

struct stScreenBuffer
{
	int nBufferSize;
	unsigned int* pBufferData;

	stScreenBuffer()
	{
		nBufferSize = 0;
		pBufferData = NULL;
	}
};

struct _SwsContext  // part of SwsContext
{
	int *ptr_dummy1;
	int *ptr_dummy2;
	int srcW;
	int srcH;
	int dstH;
	int chrSrcW;
	int chrSrcH;
	int chrDstW;
	int chrDstH;
	int lumXInc, chrXInc;
	int lumYInc, chrYInc;
	AVPixelFormat dstFormat;
	AVPixelFormat srcFormat;
};

//Not Efficient, Just an example
//change endian of a pixel (32bit)
inline void CHANGE_ENDIAN_32(unsigned char *data){
	char temp3, temp2;
	temp3 = data[3];
	temp2 = data[2];
	data[3] = data[0];
	data[2] = data[1];
	data[0] = temp3;
	data[1] = temp2;
}

//change endian of a pixel (24bit)
inline void CHANGE_ENDIAN_24(unsigned char *data){
	char temp2 = data[2];
	data[2] = data[0];
	data[0] = temp2;
}

//RGBA to RGB24 (or BGRA to BGR24)
inline void CONVERT_RGBA32toRGB24(unsigned char *image, int w, int h){
	for (int i = 0; i<h; i++)
	for (int j = 0; j<w; j++){
		memcpy(image + (i*w + j) * 3, image + (i*w + j) * 4, 3);
	}
}

//RGB24 to BGR24
inline void CONVERT_RGB24toBGR24(unsigned char *image, int w, int h){
	for (int i = 0; i<h; i++)
	for (int j = 0; j<w; j++){
		char temp2;
		temp2 = image[(i*w + j) * 3 + 2];
		image[(i*w + j) * 3 + 2] = image[(i*w + j) * 3 + 0];
		image[(i*w + j) * 3 + 0] = temp2;
	}
}

//Change endian of a picture
inline void CHANGE_ENDIAN_PIC(unsigned char *image, int w, int h, int bpp){
	unsigned char *pixeldata = NULL;
	for (int i = 0; i<h; i++)
	for (int j = 0; j<w; j++){
		pixeldata = image + (i*w + j)*bpp / 8;
		if (bpp == 32){
			CHANGE_ENDIAN_32(pixeldata);
		}
		else if (bpp == 24){
			CHANGE_ENDIAN_24(pixeldata);
		}
	}
}

inline unsigned char CONVERT_ADJUST(double tmp)
{
	return (unsigned char)((tmp >= 0 && tmp <= 255) ? tmp : (tmp < 0 ? 0 : 255));
}

//YUV420P to RGB24
inline void CONVERT_YUV420PtoRGB24(unsigned char* yuv_src, unsigned char* rgb_dst, int nWidth, int nHeight)
{
	unsigned char *tmpbuf = (unsigned char *)malloc(nWidth*nHeight * 3);
	unsigned char Y, U, V, R, G, B;
	unsigned char* y_planar, *u_planar, *v_planar;
	int rgb_width, u_width;
	rgb_width = nWidth * 3;
	u_width = (nWidth >> 1);
	int ypSize = nWidth * nHeight;
	int upSize = (ypSize >> 2);
	int offSet = 0;

	y_planar = yuv_src;
	u_planar = yuv_src + ypSize;
	v_planar = u_planar + upSize;

	for (int i = 0; i < nHeight; i++)
	{
		for (int j = 0; j < nWidth; j++)
		{
			// Get the Y value from the y planar
			Y = *(y_planar + nWidth * i + j);
			// Get the V value from the u planar
			offSet = (i >> 1) * (u_width)+(j >> 1);
			V = *(u_planar + offSet);
			// Get the U value from the v planar
			U = *(v_planar + offSet);

			// Cacular the R,G,B values
			// Method 1
			R = CONVERT_ADJUST((Y + (1.4075 * (V - 128))));
			G = CONVERT_ADJUST((Y - (0.3455 * (U - 128) - 0.7169 * (V - 128))));
			B = CONVERT_ADJUST((Y + (1.7790 * (U - 128))));
			/*
			// The following formulas are from MicroSoft' MSDN
			int C,D,E;
			// Method 2
			C = Y - 16;
			D = U - 128;
			E = V - 128;
			R = CONVERT_ADJUST(( 298 * C + 409 * E + 128) >> 8);
			G = CONVERT_ADJUST(( 298 * C - 100 * D - 208 * E + 128) >> 8);
			B = CONVERT_ADJUST(( 298 * C + 516 * D + 128) >> 8);
			R = ((R - 128) * .6 + 128 )>255?255:(R - 128) * .6 + 128;
			G = ((G - 128) * .6 + 128 )>255?255:(G - 128) * .6 + 128;
			B = ((B - 128) * .6 + 128 )>255?255:(B - 128) * .6 + 128;
			*/

			offSet = rgb_width * i + j * 3;

			rgb_dst[offSet] = B;
			rgb_dst[offSet + 1] = G;
			rgb_dst[offSet + 2] = R;
		}
	}
	free(tmpbuf);
}

inline int AVFrameToRGBACBitmap(AVFrame* frame, AVPicture* pPic)
{
	int nRet = 0;
	struct SwsContext* pSwsCtx = NULL;

	if (NULL == frame)
	{
		return -1;
	}

	//为图片分配置空间
	nRet = avpicture_alloc(pPic, AV_PIX_FMT_BGRA, frame->width, frame->height);
	if (nRet)
	{
		TRACE(_T("avpicture_alloc fail:%d\n"), nRet);
		return nRet;
	}

	//如果模式相同,则不需要转换
	if (AV_PIX_FMT_BGRA == frame->format)
	{
		memcpy(pPic->data[0], frame->data[0], frame->linesize[0]);
		pPic->linesize[0] = frame->linesize[0];
		return 0;
	}

	//设置图像转换上下文  
	pSwsCtx = sws_getCachedContext(NULL,
		frame->width,                //源宽度
		frame->height,               //源高度
		(AVPixelFormat)frame->format,//源格式
		frame->width,                //目标宽度
		frame->height,               //目标高度
		AV_PIX_FMT_BGRA,             //目的格式
		SWS_BICUBIC,                 //转换算法
		NULL, NULL, NULL);
	if (NULL == pSwsCtx)
	{
		TRACE(_T("sws_getContext false\n"));
		avpicture_free(pPic);
		return -3;
	}

	//进行图片转换
	nRet = sws_scale(pSwsCtx,
		frame->data, frame->linesize,
		0, frame->height,
		pPic->data, pPic->linesize);
	if (nRet < 0)
		TRACE(_T("sws_scale fail:%d\n"), nRet);

	sws_freeContext(pSwsCtx);

	return nRet;
}

inline void PushScreenBuffer(std::deque<stScreenBuffer>&stScrBuff, const stScreenBuffer scrBuff)
{
	if ((int)stScrBuff.size() >= MAX_BUFFER_SIZE)
		return;
	stScreenBuffer stPushData;
	stPushData.nBufferSize = scrBuff.nBufferSize;
	stPushData.pBufferData = scrBuff.pBufferData;

	stScrBuff.push_back(stPushData);
}

static SwsContext*	convert_ctx = NULL;
//in_buf is a rbg format pure memory buffer, pFrame_out is a yuv420P format AVFrame
inline bool ConvertFormat(void* in_buf, int in_w, int in_h, AVPixelFormat in_fmt, AVFrame* pFrame_out, int o_w, int o_h, AVPixelFormat o_fmt)
{
	AVFrame* pFrame_in = av_frame_alloc();
	//av_image_fill_arrays(pFrame_in->data, pFrame_in->linesize, (uint8_t*)in_buf, in_fmt,
	//	in_w, in_h, 1);
	int nRet = avpicture_fill((AVPicture*)pFrame_in, (uint8_t*)in_buf, in_fmt, in_w, in_h);

	nRet = avpicture_alloc((AVPicture*)pFrame_out, o_fmt, o_w, o_h);

	pFrame_out->width = o_w;
	pFrame_out->height = o_h;
	pFrame_out->format = o_fmt;
	
	int in_stride[4];
	av_image_fill_linesizes(in_stride, in_fmt, in_w);

	// sws_getCachedContext cause memory leak
	//SwsContext*	convert_ctx = NULL;
	if (convert_ctx)
	{
		_SwsContext* convert_dummy = (_SwsContext*)convert_ctx;
		if (convert_dummy->srcW != in_w || convert_dummy->srcH != in_h || convert_dummy->srcFormat != in_fmt
			|| convert_dummy->dstH != o_h || convert_dummy->dstFormat != o_fmt)
		{
			sws_freeContext(convert_ctx);
			convert_ctx = NULL;
		}
	}
	if (!convert_ctx)
		convert_ctx = sws_getCachedContext(convert_ctx, in_w, in_h, in_fmt, o_w, o_h, o_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	if (!convert_ctx)
		return false;

	//uint8_t *p_in[4] = {(uint8_t*)in_buf, 0, 0, 0};
	int ret = sws_scale(convert_ctx, pFrame_in->data, in_stride, 0, in_h, pFrame_out->data, pFrame_out->linesize);
	//assert(ret == in_h);
	
	av_frame_free(&pFrame_in);

	return true;
}

inline void PushAVFrame(std::deque<AVFrame*>& dequeAVFrame, AVFrame* avFrame)
{
	if ((int)dequeAVFrame.size() >= MAX_BUFFER_SIZE)
	{
		av_frame_free(&avFrame);
		return;
	}
	dequeAVFrame.push_back(avFrame);
}

inline int PushRenderBuffer(std::deque<void*>& dequeBuffer, void* pBuffer)
{
	if ((int)dequeBuffer.size() >= MAX_BUFFER_SIZE)
	{
		return -1;
	}
	dequeBuffer.push_back(pBuffer);
	return 0;
}

inline int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (fmt_ctx == NULL || stream_index < 0)
		return -1;

	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;
	while (1) {
		printf("Flushing stream #%u encoder\n", stream_index);
		//ret = encode_write_frame(NULL, stream_index, &got_frame);
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{
			ret = 0; break;
		}
		printf("编码成功1帧！\n");
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}