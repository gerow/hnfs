require 'sinatra'
require 'hnscraper'
require 'json'
require 'redis'

scraper = HNScraper.new
redis = Redis.new

crawl_delay = 30
# hn's robots.txt lists a crawl delay of 30 seconds.
# Since we might also need to request comments for
# articles we cache 4x as long. We may ocasionally
# go over the crawl delay, but this should serve
# as an ok buffer.

get '/' do
  content_type 'application/json'
  content = redis.get('front_page')
  if not content
    content = scraper.front_page.to_json
    redis.set 'front_page', content
    redis.expire 'front_page', crawl_delay * 4
  end
  cache_control :public, :max_age => redis.ttl('front_page')
  puts "Cache ttl is #{redis.ttl 'front_page'}"
  content
end
