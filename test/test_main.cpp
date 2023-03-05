#include <quickgui/widgets.hpp>

void RegisterAppMinimalTests(ImGuiTestEngine* e);
void draw_hello_world();

int main()
{
    quickgui::widgets::WidgetState st;
    quickgui::GuiConfig gcfg = {};
    if (!quickgui::gui_init(gcfg))
        return -1;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Normal;
    // screen capture
    //test_io.ScreenCaptureFunc = ImGuiApp_ScreenCaptureFunc;
    //test_io.ScreenCaptureUserData = (void*)app;
    // Optional: save test output in junit-compatible XML format.
    //test_io.ExportResultsFile = "./results.xml";
    //test_io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;

    // Start test engine
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();

    // Register tests
    RegisterAppMinimalTests(engine);
    while (true)
    {
        if(!quickgui::gui_start_frame())
            break;
        // do work here
        {
            // ...
        }
        // draw the gui
        {
            quickgui::widgets::events(&st);
            quickgui::widgets::draw_bg();
            ImGuiTestEngine_ShowTestEngineWindows(engine, NULL);
            draw_hello_world();
            quickgui::widgets::vulkan_window(&st);
            #ifdef QUICKGUI_WITH_DEMOS
            quickgui::widgets::demo(&st);
            #endif
        }
        quickgui::gui_end_frame(st.clear_color);
    }
    quickgui::gui_terminate();
}

void draw_hello_world()
{
    static bool show_hello_world = false;
    ImGui::Begin("Hello");
    if(ImGui::Button("HELLO!"))
        show_hello_world = true;
    quickgui::widgets::set_hover_tooltip("click me!");
    ImGui::Text("Press Ctrl-D for debug window");
    ImGui::End();
    if(show_hello_world)
    {
        ImGui::Begin("Hello World", &show_hello_world);
        ImGui::Text("HELLO WORLD!");
        if(ImGui::Button("bye."))
            show_hello_world = false;
        quickgui::widgets::set_hover_tooltip("close this window");
        ImGui::End();
    }
}


void RegisterAppMinimalTests(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    //-----------------------------------------------------------------
    // ## Demo Test: Hello Automation World
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "test1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        IM_UNUSED(ctx);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello, automation world");
        ImGui::Button("Click Me");
        if (ImGui::TreeNode("Node"))
        {
            static bool b = false;
            ImGui::Checkbox("Checkbox", &b);
            ImGui::TreePop();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemClick("Click Me");
        ctx->ItemOpen("Node"); // Optional as ItemCheck("Node/Checkbox") can do it automatically
        ctx->ItemCheck("Node/Checkbox");
        ctx->ItemUncheck("Node/Checkbox");
    };

    //-----------------------------------------------------------------
    // ## Demo Test: Use variables to communicate data between GuiFunc and TestFunc
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "test2");
    struct TestVars2 { int MyInt = 42; };
    t->SetVarsDataType<TestVars2>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TestVars2& vars = ctx->GetVars<TestVars2>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::SliderInt("Slider", &vars.MyInt, 0, 1000);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TestVars2& vars = ctx->GetVars<TestVars2>();
        ctx->SetRef("Test Window");

        IM_CHECK_EQ(vars.MyInt, 42);
        ctx->ItemInputValue("Slider", 123);
        IM_CHECK_EQ(vars.MyInt, 123);
    };

    //-----------------------------------------------------------------
    // ## Open Metrics window
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "open_metrics");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/Metrics\\/Debugger");
    };

    //-----------------------------------------------------------------
    // ## Capture entire Dear ImGui Demo window.
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "capture_screenshot");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Widgets");       // Open collapsing header
        ctx->ItemOpenAll("Basic");      // Open tree node and all its descendant
        ctx->CaptureScreenshotWindow("Dear ImGui Demo", ImGuiCaptureFlags_StitchAll | ImGuiCaptureFlags_HideMouseCursor);
    };

    t = IM_REGISTER_TEST(e, "demo_tests", "capture_video");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MouseTeleportToPos(ctx->GetWindowByRef("")->Pos);

        ctx->CaptureAddWindow("Dear ImGui Demo"); // Optional: Capture single window
        ctx->CaptureBeginVideo();
        ctx->ItemOpen("Widgets");
        ctx->ItemInputValue("Basic/input text", "My first video!");
        ctx->CaptureEndVideo();
    };


}
