require 'sinatra'
require 'hnscraper'

scraper = HNScraper.new
redis = Redis.new

get '/' do
  content_type 'application/json'
  scraper.front_page
end
