class LatheParameter {

    public:
        LatheParameter();

        int backlash();
        void setBacklash(int value);

        int feedIndex();
        void setFeedIndex(int value);

        bool isMetricFeed();
        void setMetricFeed(bool value);

        bool isInvertFeed();
        void setInvertFeed(bool value);

    private:
        int backlashValue;
        int feedIndexValue;
        bool metricFeedValue;
        bool invertFeedValue;
};