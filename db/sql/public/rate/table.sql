--------------------------------------------------------------------------------
-- RATE ------------------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE TABLE public.rate (
    id			    uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    currencyFrom    uuid NOT NULL REFERENCES public.currency(id) ON DELETE RESTRICT,
    currencyTo      uuid NOT NULL REFERENCES public.currency(id) ON DELETE RESTRICT,
    value		    numeric NOT NULL,
    validFromDate	timestamptz DEFAULT Now() NOT NULL,
    validToDate		timestamptz DEFAULT public.max_date() NOT NULL
);

COMMENT ON TABLE public.rate IS 'Соотношения между валютами для задания курсов валют';

COMMENT ON COLUMN public.rate.id IS 'Идентификатор';
COMMENT ON COLUMN public.rate.currencyFrom IS 'Ссылка на первую валюту';
COMMENT ON COLUMN public.rate.currencyTo IS 'Ссылка на вторую валюту';
COMMENT ON COLUMN public.rate.value IS 'Значение - соотношение между первой и второй валютой';
COMMENT ON COLUMN public.rate.validFromDate IS 'Дата начала периода действия';
COMMENT ON COLUMN public.rate.validToDate IS 'Дата окончания периода действия';

--------------------------------------------------------------------------------

CREATE INDEX ON public.rate (currencyFrom);
CREATE INDEX ON public.rate (currencyTo);

CREATE UNIQUE INDEX ON public.rate (currencyFrom, currencyTo, validFromDate, validToDate);
