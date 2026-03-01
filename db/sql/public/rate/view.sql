--------------------------------------------------------------------------------
-- VIEW public.rates -----------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE VIEW public.rates
AS
  SELECT t.id,
         t.currencyfrom, c1.code AS currencyFromCode, c1.name AS currencyFromName,
         t.currencyto, c2.code AS currencyToCode, c2.name AS currencyToName,
         value, validfromdate, validtodate
    FROM public.rate t INNER JOIN public.currency c1 ON t.currencyFrom = c1.id
                       INNER JOIN public.currency c2 ON t.currencyTo = c2.id;

GRANT SELECT ON public.rates TO :username;
