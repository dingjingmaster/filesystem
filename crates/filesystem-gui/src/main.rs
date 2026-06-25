mod app;
mod components;
mod config;
mod icons;
mod model;
mod style;
mod tasks;
mod utils;

use app::FileManager;
use config::window_settings;
use iced::{Result as IcedResult, Theme};

pub fn main() -> IcedResult {
    iced::application(FileManager::new, FileManager::update, FileManager::view)
        .title(FileManager::title)
        .theme(Theme::Dark)
        .window(window_settings())
        .subscription(FileManager::subscription)
        .run()
}
