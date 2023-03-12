#include "quickgui/cvutil/cvutil.hpp"

namespace quickgui {

namespace {
static inline constexpr cvtypespecs cvtypes[] = {
    #define _(ty, dt, nc) cvtypespecs{ty, imgview::data_##dt, nc, #ty}
        _(CV_8U   , u8, 1),
        _(CV_8UC1 , u8, 1),
        _(CV_8UC2 , u8, 2),
        _(CV_8UC3 , u8, 3),
        _(CV_8UC4 , u8, 4),
        //
        _(CV_8S   , i8, 1),
        _(CV_8SC1 , i8, 1),
        _(CV_8SC2 , i8, 2),
        _(CV_8SC3 , i8, 3),
        _(CV_8SC4 , i8, 4),
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
    return imgview::data_type_size(ts.data_type) * ts.num_channels;
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

const cv::Mat cvmat(imgview const& view)
{
    cv::Size matsize(/*cols*/(int)view.width, /*rows*/(int)view.height);
    const cv::Mat matview(matsize, cvformat(view), view.buf);
    // ensure that the cvmat is pointing at our data
    C4_ASSERT(same_mat(matview, view));
    return matview;
}

cv::Mat cvmat(imgview & view)
{
    cv::Size matsize(/*cols*/(int)view.width, /*rows*/(int)view.height);
    cv::Mat matview(matsize, cvformat(view), view.buf);
    // ensure that the cvmat is pointing at our data
    C4_ASSERT(same_mat(matview, view));
    return matview;
}

bool same_mat(cv::Mat const& mat, imgview const& view)
{
    bool ok = true;
    ok &= (mat.type() == cvformat(view));
    ok &= (mat.isContinuous());
    ok &= (&mat.at<char>(0) == view.buf);
    ok &= (mat.total() * mat.elemSize() <= view.size_bytes);
    return ok;
}

} // namespace quickgui
