#include "quickgui/cvutil/cvutil.hpp"

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

namespace quickgui {

namespace {
static inline constexpr cvtypespecs cvtypes[] = {
    #define _(ty, dt, nc) cvtypespecs{ty, imgviewtype::dt, nc, #ty}
        _(CV_8U  , u8, 1),
        _(CV_8UC1, u8, 1),
        _(CV_8UC2, u8, 2),
        _(CV_8UC3, u8, 3),
        _(CV_8UC4, u8, 4),
        //
        _(CV_8S  , i8, 1),
        _(CV_8SC1, i8, 1),
        _(CV_8SC2, i8, 2),
        _(CV_8SC3, i8, 3),
        _(CV_8SC4, i8, 4),
        //
        _(CV_16U  , u16, 1),
        _(CV_16UC1, u16, 1),
        _(CV_16UC2, u16, 2),
        _(CV_16UC3, u16, 3),
        _(CV_16UC4, u16, 4),
        //
        _(CV_16S  , i16, 1),
        _(CV_16SC1, i16, 1),
        _(CV_16SC2, i16, 2),
        _(CV_16SC3, i16, 3),
        _(CV_16SC4, i16, 4),
        //
        _(CV_32S  , i32, 1),
        _(CV_32SC1, i32, 1),
        _(CV_32SC2, i32, 2),
        _(CV_32SC3, i32, 3),
        _(CV_32SC4, i32, 4),
        //
        _(CV_32F  , f32, 1),
        _(CV_32FC1, f32, 1),
        _(CV_32FC2, f32, 2),
        _(CV_32FC3, f32, 3),
        _(CV_32FC4, f32, 4),
        //
        _(CV_64F  , f64, 1),
        _(CV_64FC1, f64, 1),
        _(CV_64FC2, f64, 2),
        _(CV_64FC3, f64, 3),
        _(CV_64FC4, f64, 4),
    #undef _
};
} // namespace


cvtypespecs const& cvtype_lookup(int cvtypeint)
{
    for(auto const& cvt : cvtypes)
        if(cvtypeint == cvt.val)
            return cvt;
    C4_ERROR("format type not implemented: %d", cvtypeint);
    return cvtypes[0];
}

cvtypespecs const& cvtype_lookup(imgview const& view)
{
    for(auto const& cvt : cvtypes)
        if(cvt.data_type == view.data_type && cvt.num_channels == view.num_channels)
            return cvt;
    C4_ERROR("data type not found/implemented");
    return cvtypes[0];
}

int cvformat(imgview const& view)
{
    for(auto const& cvt : cvtypes)
        if(cvt.data_type == view.data_type
           && cvt.num_channels == view.num_channels)
            return cvt.val;
    C4_ERROR("format type not implemented");
    return cvtypes[0].val;
}

c4::csubstr cvtype_str(int cvtypeint)
{
    return c4::to_csubstr(cvtype_lookup(cvtypeint).str);
}

size_t cvtype_bytes(int cvtypeint)
{
    cvtypespecs ts = cvtype_lookup(cvtypeint);
    return imgviewtype::data_size(ts.data_type) * ts.num_channels;
}

int cvtype_to_video(int cvtypeint)
{
    switch(cvtypeint)
    {
    case CV_8U: return CV_8UC3;
    case CV_8S: return CV_8SC3;
    case CV_32S: return CV_32SC3;
    case CV_32F: return CV_32FC3;
    }
    return cvtypeint;
}

cv::Mat cvmat(imgview const& view)
{
    cv::Size matsize(/*cols*/(int)view.width, /*rows*/(int)view.height);
    C4_SUPPRESS_WARNING_GCC_CLANG_WITH_PUSH("-Wcast-qual")
    const cv::Mat matview(matsize, cvformat(view), (char*)view.buf);
    C4_SUPPRESS_WARNING_GCC_CLANG_POP
    // ensure that the cvmat is pointing at our data
    C4_ASSERT(same_mat(matview, view));
    return matview;
}

cv::Mat cvmat(wimgview const& view)
{
    cv::Size matsize(/*cols*/(int)view.width, /*rows*/(int)view.height);
    cv::Mat matview(matsize, cvformat(view), view.buf);
    // ensure that the cvmat is pointing at our data
    C4_ASSERT(same_mat(matview, view));
    return matview;
}

imgview cvmat_to_imgview(const cv::Mat& mat)
{
    C4_CHECK(mat.dims == 2);
    C4_CHECK(mat.isContinuous());
    cvtypespecs const& specs = cvtype_lookup(mat.type());
    imgview view;
    view.buf = (const uint8_t*)mat.datastart;
    view.buf_size = (size_t)(mat.dataend - mat.datastart);
    view.width = mat.cols;
    view.height = mat.rows;
    view.num_channels = specs.num_channels;
    view.data_type = specs.data_type;
    return view;
}

wimgview cvmat_to_imgview(cv::Mat& mat)
{
    C4_CHECK(mat.dims == 2);
    C4_CHECK(mat.isContinuous());
    cvtypespecs const& specs = cvtype_lookup(mat.type());
    wimgview view;
    view.buf = (uint8_t*)mat.datastart;
    view.buf_size = (size_t)(mat.dataend - mat.datastart);
    view.width = mat.cols;
    view.height = mat.rows;
    view.num_channels = specs.num_channels;
    view.data_type = specs.data_type;
    return view;
}

bool same_mat(cv::Mat const& mat, imgview const& view)
{
    bool ok = true;
    ok &= (mat.type() == cvformat(view));
    ok &= (mat.isContinuous());
    ok &= (&mat.at<char>(0) == (const char*)view.buf);
    ok &= (mat.total() * mat.elemSize() <= view.bytes_required());
    return ok;
}

} // namespace quickgui

C4_SUPPRESS_WARNING_GCC_CLANG_POP
