#include "deskmate/app/app.h"

#include "deskmate/app/config.h"
#include "deskmate/gfx/components/mqtt_circle_horizontal_list_item.h"
#include "deskmate/gfx/components/mqtt_list_item.h"
#include "deskmate/gfx/components/mqtt_vertical_bar_list_item.h"
#include "deskmate/gfx/components/text_list_item.h"
#include "deskmate/gfx/display.h"
#include "deskmate/gfx/screens/horizontal_list.h"
#include "deskmate/gfx/screens/list.h"
#include "deskmate/gfx/screens/vertical_bars_list.h"
#include "deskmate/gfx/screens/window.h"
#include "deskmate/input/input.h"
#include "deskmate/mqtt/mqtt.h"

namespace deskmate {
namespace app {
namespace {
using deskmate::app::MQTTConfig;
using deskmate::app::MQTTFloatingPointSensorConfig;
using deskmate::gfx::Display;
using deskmate::gfx::Size;
using deskmate::gfx::components::MQTTCircleHorizontalListItem;
using deskmate::gfx::components::MQTTListItem;
using deskmate::gfx::components::MQTTVerticalBarListItem;
using deskmate::gfx::components::TextListItem;
using deskmate::gfx::screens::HorizontalList;
using deskmate::gfx::screens::HorizontalListItem;
using deskmate::gfx::screens::ListItem;
using deskmate::gfx::screens::ListScreen;
using deskmate::gfx::screens::VerticalBarListItem;
using deskmate::gfx::screens::VerticalBarsList;
using deskmate::gfx::screens::Window;
using deskmate::gfx::screens::WindowedScreen;
using deskmate::input::InputEventHandler;
using deskmate::mqtt::MQTTMessage;
using deskmate::mqtt::MQTTMessageBuffer;
using deskmate::mqtt::MQTTSubscriber;

std::unique_ptr<ListScreen> MakeSwitchesControls(
    const std::vector<MQTTConfig> &mqtt_configs,
    MQTTMessageBuffer *mqtt_buffer) {
  std::vector<std::unique_ptr<ListItem>> left_list_items;
  for (const auto &cfg : mqtt_configs) {
    std::unique_ptr<MQTTListItem> list_item = std::make_unique<MQTTListItem>(
        cfg.display_name, cfg.command_topic, cfg.state_topic, mqtt_buffer);
    mqtt_buffer->Subscribe(list_item.get());
    left_list_items.push_back(std::move(list_item));
  }
  return std::make_unique<ListScreen>(left_list_items);
}

std::unique_ptr<VerticalBarsList> MakePlantsDashboard(
    const std::vector<MQTTFloatingPointSensorConfig> &sensor_configs,
    MQTTMessageBuffer *mqtt_buffer) {
  std::vector<std::unique_ptr<VerticalBarListItem>> items;
  for (const auto &config : sensor_configs) {
    auto item = std::make_unique<MQTTVerticalBarListItem>(
        config.display_name, config.value_topic, config.availability_topic);
    mqtt_buffer->Subscribe(item.get());
    items.push_back(std::move(item));
  }
  return std::make_unique<VerticalBarsList>(items);
}

std::unique_ptr<HorizontalList> MakeWeatherDashboard(
    const std::vector<MQTTFloatingPointSensorConfig> &weather_configs,
    MQTTMessageBuffer *mqtt_buffer) {
  std::vector<std::unique_ptr<HorizontalListItem>> items;
  for (const auto &config : weather_configs) {
    auto item = std::make_unique<MQTTCircleHorizontalListItem>(
        config.display_name, config.unit, config.value_topic,
        config.availability_topic);
    mqtt_buffer->Subscribe(item.get());
    items.push_back(std::move(item));
  }
  return std::make_unique<HorizontalList>(items);
}

}  // namespace

bool App::Init(
    const std::vector<MQTTConfig> &mqtt_configs,
    const std::vector<MQTTFloatingPointSensorConfig>& sensor_configs,
    const std::vector<MQTTFloatingPointSensorConfig>& weather_configs) {
  std::unique_ptr<VerticalBarsList> plants_dashboard =
      MakePlantsDashboard(sensor_configs, mqtt_buffer_);

  const Size &size = display_->GetSize();

  std::vector<WindowedScreen> windowed_screens;
  windowed_screens.push_back({MakeSwitchesControls(mqtt_configs, mqtt_buffer_),
                              {{0, 0}, {size.height, size.width / 2}},
                              /*focusable=*/true});
  windowed_screens.push_back(
      {std::move(plants_dashboard),
       {{0, size.width / 2}, {size.height / 2, size.width / 2}},
       /*focusable=*/true});
  windowed_screens.push_back(
      {MakeWeatherDashboard(weather_configs, mqtt_buffer_),
       {{size.height / 2, size.width / 2}, {size.height / 2, size.width / 2}},
       /*focusable=*/true});

  window_ = std::make_unique<Window>(windowed_screens);
  return true;
}

bool App::Tick() {
  mqtt_buffer_->Tick();
  display_->Clear();
  window_->Render(display_);
  display_->Refresh();
  return true;
}

}  // namespace app
}  // namespace deskmate