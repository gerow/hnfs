require 'redis'
require 'nokogiri'
require 'open-uri'
require 'json'

class HNScraper
  # This is all super hacky, but really what scraper isn't?
  def initialize params={}
    @redis = Redis.new
    @url = params[:url] || "https://news.ycombinator.com/"
  end

  def front_page
    if not @redis.get('front_page')
      doc = Nokogiri::HTML(open(@url))
      p doc
      sliced_story_xmls = doc.css("table>tr>td>table>tr").each_slice(3).to_a[0..29]
      stories = sliced_story_xmls.map{|story_xmls|
        story = {}
        story[:url] = story_xmls[1].css("td.title>a").attr("href").value
        story[:title] = story_xmls[1].css("td.title>a").text
        story
      }
      @redis.set('front_page', stories.to_json)
      @redis.expire('front_page', 10)
    end
    return @redis.get('front_page')
  end
end
