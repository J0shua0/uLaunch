#include <ui/ui_SettingsMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::ProcessedTheme theme;
extern cfg::Config config;

namespace ui
{
    template<typename T>
    std::string EncodeForSettings(T t)
    {
        return "<unknown>";
    }

    template<>
    std::string EncodeForSettings<std::string>(std::string t)
    {
        return "\"" + t + "\"";
    }
    
    template<>
    std::string EncodeForSettings<u32>(u32 t)
    {
        return "\"" + std::to_string(t) + "\"";
    }

    template<>
    std::string EncodeForSettings<bool>(bool t)
    {
        return t ? "True" : "False";
    }

    SettingsMenuLayout::SettingsMenuLayout()
    {
        this->SetBackgroundImage(cfg::ProcessedThemeResource(theme, "ui/Background.png"));

        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->settingsMenu = pu::ui::elm::Menu::New(200, 160, 880, menubgclr, 100, 4);
        this->settingsMenu->SetOnFocusColor(menufocusclr);
        qapp->ApplyConfigForElement("settings_menu", "settings_menu_item", this->settingsMenu);
        this->Add(this->settingsMenu);

        this->SetOnInput(std::bind(&SettingsMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void SettingsMenuLayout::Reload()
    {
        this->settingsMenu->ClearItems();
        char consolename[SET_MAX_NICKNAME_SIZE] = {};
        setsysGetDeviceNickname(consolename);
        this->PushSettingItem("Console nickname", EncodeForSettings<std::string>(consolename), 0);
        this->PushSettingItem("PC viewer USB enabled", EncodeForSettings(config.viewer_usb_enabled), 1);
        this->PushSettingItem("Homebrew-as-application 'flog' takeover enabled", EncodeForSettings(config.system_title_override_enabled), 2);
    }

    void SettingsMenuLayout::PushSettingItem(std::string name, std::string value_display, u32 id)
    {
        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        auto itm = pu::ui::elm::MenuItem::New(name + ": " + value_display);
        itm->AddOnClick(std::bind(&SettingsMenuLayout::setting_Click, this, id));
        itm->SetColor(textclr);
        this->settingsMenu->AddItem(itm);
    }

    void SettingsMenuLayout::setting_Click(u32 id)
    {
        bool reload_need = false;
        switch(id)
        {
            case 0:
            {
                SwkbdConfig swkbd;
                swkbdCreate(&swkbd, 0);
                swkbdConfigSetHeaderText(&swkbd, "Enter new console nickname");
                char consolename[SET_MAX_NICKNAME_SIZE] = {};
                setsysGetDeviceNickname(consolename);
                swkbdConfigSetInitialText(&swkbd, consolename);
                swkbdConfigSetStringLenMax(&swkbd, 32);
                char name[SET_MAX_NICKNAME_SIZE] = {0};
                auto rc = swkbdShow(&swkbd, name, SET_MAX_NICKNAME_SIZE);
                swkbdClose(&swkbd);
                if(R_SUCCEEDED(rc))
                {
                    setsysSetDeviceNickname(name);
                    reload_need = true;
                }
                break;
            }
            case 1:
            {
                std::string info = "uLaunch must have this option enabled to be able to stream the console's screen to QForegroundViewer PC tool.\nIf you won't use it, you can keep it disabled.\nNote that if it's enabled USB homebrew like Goldleaf might fail.\n\nWould you really like to " + std::string(config.viewer_usb_enabled ? "disable" : "enable") + " it?";
                auto sopt = qapp->CreateShowDialog("PC viewer USB", info, { "Yes", "Cancel" }, true);
                if(sopt == 0)
                {
                    config.viewer_usb_enabled = !config.viewer_usb_enabled;
                    reload_need = true;
                    qapp->CreateShowDialog("PC viewer USB", "Done. Note that you need to reboot to apply the changes.", { "Ok" }, true);
                }
                break;
            }
            case 2:
            {
                std::string info = "uLaunch will allow you to launch homebrew directly as applications this way.\nNote that this option might involve ban risk, so it is disabled by default.\n\nWould you really like to " + std::string(config.system_title_override_enabled ? "disable" : "enable") + " it?";
                auto sopt = qapp->CreateShowDialog("Homebrew 'flog' takeover", info, { "Yes", "Cancel" }, true);
                if(sopt == 0)
                {
                    config.system_title_override_enabled = !config.system_title_override_enabled;
                    reload_need = true;
                }
                break;
            }
        }
        if(reload_need) this->Reload();
    }

    void SettingsMenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        bool ret = false;
        auto [rc, msg] = am::QMenu_GetLatestQMenuMessage();
        switch(msg)
        {
            case am::QMenuMessage::HomeRequest:
            {
                ret = true;
                break;
            }
            default:
                break;
        }
        if(down & KEY_B) ret = true;
        if(ret)
        {
            qapp->FadeOut();
            qapp->LoadMenu();
            qapp->FadeIn();
        }
    }
}