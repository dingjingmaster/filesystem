mod app;
mod apps;
mod components;
mod config;
mod icons;
mod model;
mod renderer;
mod style;
mod tasks;
mod utils;

use app::FileManager;
use config::window_settings;
use iced::{Result as IcedResult, Theme};

pub fn main() -> IcedResult {
    renderer::configure_backend();

    iced::application(FileManager::new, FileManager::update, FileManager::view)
        .title(FileManager::title)
        .theme(Theme::Dark)
        .window(window_settings())
        .subscription(FileManager::subscription)
        .run()
}
